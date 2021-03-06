/*
 *  Copyright (c) Microsoft Corporation
 *  SPDX-License-Identifier: MIT
*/

/*++

Abstract:

   This file implements the classifyFn, notifiFn, and flowDeleteFn callout
   functions for the l2 callout.

Environment:

    Kernel mode

--*/

#include <ntddk.h>

#pragma warning(push)
#pragma warning(disable:4201)       // unnamed struct/union

#include <fwpsk.h>

#pragma warning(pop)

#include <fwpmk.h>
#include <netiodef.h>

#include "ebpf_l2_hook.h"

#define INITGUID
#include <guiddef.h>
#include "protocol.h"
#include "ebpf_core.h"

// XDP like hook
typedef struct _xdp_md {
    uint64_t                      data;                 /*     0     8 */
    uint64_t                      data_end;             /*     8     8 */
    uint64_t                      data_meta;            /*     16    8 */

    /* size: 12, cachelines: 1, members: 3 */
    /* last cacheline: 12 bytes */
} xdp_md_t;

typedef enum _xdp_action
{
    XDP_PASS = 1,
    XDP_DROP = 2
} xdp_action_t;

// BIND hook
typedef struct _bind_md {
    uint64_t                      app_id_start;             // 0,8
    uint64_t                      app_id_end;               // 8,8
    uint64_t                      process_id;               // 16,8
    uint8_t                       socket_address[16];       // 24,16
    uint8_t                       socket_address_length;    // 40,1
    uint8_t                       operation;                // 41,1
    uint8_t                       protocol;                 // 42,1
} bind_md_t;

typedef enum _bind_operation
{
    BIND_OPERATION_BIND,          // Entry to bind
    BIND_OPERATION_POST_BIND,     // After port allocation
    BIND_OPERATION_UNBIND,        // Release port
} bind_operation_t;

typedef enum _bind_action
{
    BIND_PERMIT,
    BIND_DENY,
    BIND_REDIRECT,
} bind_action_t;

typedef DWORD(__stdcall* bind_hook_function) (PVOID);

#define RTL_COUNT_OF(arr) (sizeof(arr) / sizeof(arr[0]))

// Callout and sublayer GUIDs

// 7c7b3fb9-3331-436a-98e1-b901df457fff
DEFINE_GUID(
    EBPF_HOOK_SUBLAYER,
    0x7c7b3fb9,
    0x3331,
    0x436a,
    0x98, 0xe1, 0xb9, 0x01, 0xdf, 0x45, 0x7f, 0xff
);

// 5a5614e5-6b64-4738-8367-33c6ca07bf8f
DEFINE_GUID(
    EBPF_HOOK_L2_CALLOUT,
    0x5a5614e5,
    0x6b64,
    0x4738,
    0x83, 0x67, 0x33, 0xc6, 0xca, 0x07, 0xbf, 0x8f
);

// c69f4de0-3d80-457d-9aea-75faef42ec12
DEFINE_GUID(
    EBPF_HOOK_ALE_BIND_REDIRECT_CALLOUT,
    0xc69f4de0,
    0x3d80,
    0x457d,
    0x9a, 0xea, 0x75, 0xfa, 0xef, 0x42, 0xec, 0x12
);

// 732acf94-7319-4fed-97d0-41d3a18f3fa1
DEFINE_GUID(
    EBPF_HOOK_ALE_RESOURCE_ALLOCATION_CALLOUT,
    0x732acf94,
    0x7319,
    0x4fed,
    0x97, 0xd0, 0x41, 0xd3, 0xa1, 0x8f, 0x3f, 0xa1
);

// d5792949-2d91-4023-9993-3f3dd9d54b2b
DEFINE_GUID(
    EBPF_HOOK_ALE_RESOURCE_RELEASE_CALLOUT,
    0xd5792949,
    0x2d91,
    0x4023,
    0x99, 0x93, 0x3f, 0x3d, 0xd9, 0xd5, 0x4b, 0x2b
);

static void
ebpf_hook_layer_2_classify(
    _In_ const FWPS_INCOMING_VALUES* incoming_fixed_values,
    _In_ const FWPS_INCOMING_METADATA_VALUES* incoming_metadata_values,
    _Inout_opt_ void* layer_data,
    _In_opt_ const void* classify_context,
    _In_ const FWPS_FILTER* filter,
    _In_ uint64_t flow_context,
    _Inout_ FWPS_CLASSIFY_OUT* classify_output);

static void
ebpf_hook_resource_allocation_classify(
    _In_ const FWPS_INCOMING_VALUES* incoming_fixed_values,
    _In_ const FWPS_INCOMING_METADATA_VALUES* incoming_metadata_values,
    _Inout_opt_ void* layer_data,
    _In_opt_ const void* classify_context,
    _In_ const FWPS_FILTER* filter,
    _In_ uint64_t flow_context,
    _Inout_ FWPS_CLASSIFY_OUT* classify_output);

static void
ebpf_hook_resource_release_classify(
    _In_ const FWPS_INCOMING_VALUES* incoming_fixed_values,
    _In_ const FWPS_INCOMING_METADATA_VALUES* incoming_metadata_values,
    _Inout_opt_ void* layer_data,
    _In_opt_ const void* classify_context,
    _In_ const FWPS_FILTER* filter,
    _In_ uint64_t flow_context,
    _Inout_ FWPS_CLASSIFY_OUT* classify_output);

static void
ebpf_hook_no_op_flow_delete(
    _In_ uint16_t layer_id,
    _In_ uint32_t fwpm_callout_id,
    _In_ uint64_t flow_context);

static NTSTATUS
ebpf_hook_no_op_notify(
    _In_ FWPS_CALLOUT_NOTIFY_TYPE callout_notification_type,
    _In_ const GUID* filter_key,
    _Inout_ const FWPS_FILTER* filter);

typedef struct _ebpf_wfp_callout_state
{
    const GUID* callout_guid;
    const GUID* layer_guid;
    FWPS_CALLOUT_CLASSIFY_FN3 classify_fn;
    FWPS_CALLOUT_NOTIFY_FN3 notify_fn;
    FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN0 delete_fn;
    wchar_t* name;
    wchar_t* description;
    FWP_ACTION_TYPE filter_action_type;
    uint32_t assigned_callout_id;
} ebpf_wfp_callout_state_t;

static ebpf_wfp_callout_state_t _ebpf_wfp_callout_state[] =
{
{
    &EBPF_HOOK_L2_CALLOUT,
    &FWPM_LAYER_INBOUND_MAC_FRAME_ETHERNET,
    ebpf_hook_layer_2_classify,
    ebpf_hook_no_op_notify,
    ebpf_hook_no_op_flow_delete,
    L"L2 XDP Callout",
    L"L2 callout driver for eBPF at XDP-like layer",
    FWP_ACTION_CALLOUT_TERMINATING,
},
{
    &EBPF_HOOK_ALE_RESOURCE_ALLOCATION_CALLOUT,
    &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4,
    ebpf_hook_resource_allocation_classify,
    ebpf_hook_no_op_notify,
    ebpf_hook_no_op_flow_delete,
    L"Resource Allocation eBPF Callout",
    L"Resource Allocation callout driver for eBPF",
    FWP_ACTION_CALLOUT_TERMINATING,
},
{
    &EBPF_HOOK_ALE_RESOURCE_RELEASE_CALLOUT,
    &FWPM_LAYER_ALE_RESOURCE_RELEASE_V4,
    ebpf_hook_resource_release_classify,
    ebpf_hook_no_op_notify,
    ebpf_hook_no_op_flow_delete,
    L"Resource Release eBPF Callout",
    L"Resource Release callout driver for eBPF",
    FWP_ACTION_CALLOUT_TERMINATING,
},
};

// Callout globals
static HANDLE _fwp_engine_handle;

static
NTSTATUS
ebpf_hook_register_wfp_callout(
    _Inout_ ebpf_wfp_callout_state_t * callout_state,
    _Inout_ void* device_object
    )
/* ++

   This function registers callouts and filters.

-- */
{
    NTSTATUS status = STATUS_SUCCESS;

    FWPS_CALLOUT callout_register_state = { 0 };
    FWPM_CALLOUT callout_add_state = { 0 };

    FWPM_DISPLAY_DATA display_data = { 0 };
    FWPM_FILTER filter = { 0 };

    BOOLEAN was_callout_registered = FALSE;

    callout_register_state.calloutKey = *callout_state->callout_guid;
    callout_register_state.classifyFn = callout_state->classify_fn;
    callout_register_state.notifyFn = callout_state->notify_fn; 
    callout_register_state.flowDeleteFn = callout_state->delete_fn; 
    callout_register_state.flags = 0;

    status = FwpsCalloutRegister(
        device_object,
        &callout_register_state,
        &callout_state->assigned_callout_id
    );
    if (!NT_SUCCESS(status))
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Ebpf_wfp: FwpsCalloutRegister for %S failed with error %.2X\n", callout_state->name, status));
        goto Exit;
    }
    was_callout_registered = TRUE;

    display_data.name = callout_state->name;
    display_data.description = callout_state->description;

    callout_add_state.calloutKey = *callout_state->callout_guid;
    callout_add_state.displayData = display_data;
    callout_add_state.applicableLayer = *callout_state->layer_guid;

    status = FwpmCalloutAdd(
        _fwp_engine_handle,
        &callout_add_state,
        NULL,
        NULL
    );

    if (!NT_SUCCESS(status))
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Ebpf_wfp: FwpmCalloutAdd for %S failed with error %.2X\n", callout_state->name, status));
        goto Exit;
    }

    filter.layerKey = *callout_state->layer_guid;
    filter.displayData.name = callout_state->name;
    filter.displayData.description = callout_state->description;
    filter.action.type = callout_state->filter_action_type;
    filter.action.calloutKey = *callout_state->callout_guid;
    filter.filterCondition = NULL;
    filter.numFilterConditions = 0;
    filter.subLayerKey = EBPF_HOOK_SUBLAYER;
    filter.weight.type = FWP_EMPTY; // auto-weight.

    status = FwpmFilterAdd(
        _fwp_engine_handle,
        &filter,
        NULL,
        NULL);

    if (!NT_SUCCESS(status))
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Ebpf_wfp: FwpmFilterAdd for %S failed with error %.2X\n", callout_state->name, status));
        goto Exit;
    }

Exit:

    if (!NT_SUCCESS(status))
    {
        if (was_callout_registered)
        {
            FwpsCalloutUnregisterById(callout_state->assigned_callout_id);
            callout_state->assigned_callout_id = 0;
        }
    }

    return status;
}


NTSTATUS
ebpf_hook_register_callouts(
    _Inout_ void* device_object
)
/* ++

   This function registers dynamic callouts and filters that
   FWPM_LAYER_INBOUND_MAC_FRAME_ETHERNET layer.

   Callouts and filters will be removed during DriverUnload.

-- */
{
    NTSTATUS status = STATUS_SUCCESS;
    FWPM_SUBLAYER ebpf_hook_sub_layer;

    BOOLEAN is_engined_opened = FALSE;
    BOOLEAN is_in_transaction = FALSE;

    FWPM_SESSION session = { 0 };

    size_t index;

    if (_fwp_engine_handle != NULL)
    {
        // already registered
        goto Exit;
    }

    session.flags = FWPM_SESSION_FLAG_DYNAMIC;

    status = FwpmEngineOpen(
        NULL,
        RPC_C_AUTHN_WINNT,
        NULL,
        &session,
        &_fwp_engine_handle
    );
    if (!NT_SUCCESS(status))
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Ebpf_wfp: FwpmEngineOpen failed with error %.2X\n", status));
        goto Exit;
    }
    is_engined_opened = TRUE;

    status = FwpmTransactionBegin(_fwp_engine_handle, 0);
    if (!NT_SUCCESS(status))
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Ebpf_wfp: FwpmTransactionBegin failed with error %.2X\n", status));
        goto Exit;
    }
    is_in_transaction = TRUE;

    RtlZeroMemory(&ebpf_hook_sub_layer, sizeof(FWPM_SUBLAYER));

    ebpf_hook_sub_layer.subLayerKey = EBPF_HOOK_SUBLAYER;
    ebpf_hook_sub_layer.displayData.name = L"EBPF hook Sub-Layer";
    ebpf_hook_sub_layer.displayData.description =
        L"Sub-Layer for use by EBPF callouts";
    ebpf_hook_sub_layer.flags = 0;
    ebpf_hook_sub_layer.weight = FWP_EMPTY; // auto-weight.;

    status = FwpmSubLayerAdd(_fwp_engine_handle, &ebpf_hook_sub_layer, NULL);
    if (!NT_SUCCESS(status))
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Ebpf_wfp: FwpmSubLayerAdd failed with error %.2X\n", status));
        goto Exit;
    }

    for (index = 0; index < RTL_COUNT_OF(_ebpf_wfp_callout_state); index++)
    {
        status = ebpf_hook_register_wfp_callout(&_ebpf_wfp_callout_state[index], device_object);
        if (!NT_SUCCESS(status))
        {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Ebpf_wfp: ebpf_hook_register_wfp_callout failed for %S with error %.2X\n", _ebpf_wfp_callout_state[index].name, status));
            goto Exit;
        }
    }

    status = FwpmTransactionCommit(_fwp_engine_handle);
    if (!NT_SUCCESS(status))
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Ebpf_wfp: FwpmTransactionCommit failed with error %.2X\n", status));
        goto Exit;
    }
    is_in_transaction = FALSE;

Exit:

    if (!NT_SUCCESS(status))
    {
        if (is_in_transaction)
        {
            FwpmTransactionAbort(_fwp_engine_handle);
            _Analysis_assume_lock_not_held_(_fwp_engine_handle); // Potential leak if "FwpmTransactionAbort" fails
        }
        if (is_engined_opened)
        {
            FwpmEngineClose(_fwp_engine_handle);
            _fwp_engine_handle = NULL;
        }
    }

    return status;
}

void
ebpf_hook_unregister_callouts(void)
{
    size_t index;
    if (_fwp_engine_handle != NULL)
    {
        FwpmEngineClose(_fwp_engine_handle);
        _fwp_engine_handle = NULL;

        for (index = 0; index < RTL_COUNT_OF(_ebpf_wfp_callout_state); index++)
        {
            FwpsCalloutUnregisterById(_ebpf_wfp_callout_state[index].assigned_callout_id);
        }
    }
}

static void
ebpf_hook_layer_2_classify(
   _In_ const FWPS_INCOMING_VALUES* incoming_fixed_values,
   _In_ const FWPS_INCOMING_METADATA_VALUES* incoming_metadata_values,
   _Inout_opt_ void* layer_data,
   _In_opt_ const void* classify_context,
   _In_ const FWPS_FILTER* filter,
   _In_ uint64_t flow_context,
   _Inout_ FWPS_CLASSIFY_OUT* classify_output
   )
/* ++

   A simple classify function at the WFP L2 MAC layer.

-- */
{
   FWP_ACTION_TYPE action = FWP_ACTION_PERMIT;
   UNREFERENCED_PARAMETER(incoming_fixed_values);
   UNREFERENCED_PARAMETER(incoming_metadata_values);
   UNREFERENCED_PARAMETER(classify_context);
   UNREFERENCED_PARAMETER(filter);
   UNREFERENCED_PARAMETER(flow_context);
   NET_BUFFER_LIST* nbl = (NET_BUFFER_LIST*)layer_data;
   NET_BUFFER* net_buffer = NULL;
   uint8_t* packet_buffer;
   uint32_t result = 0;

   if (nbl == NULL)
   {
       KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Null nbl \n"));
       goto done;
   }

   net_buffer = NET_BUFFER_LIST_FIRST_NB(nbl);
   if (net_buffer == NULL)
   {
       KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "net_buffer not present\n"));
       // nothing to do
       goto done;
   }


   packet_buffer =
       NdisGetDataBuffer(
           net_buffer,
           net_buffer->DataLength,
           NULL,
           sizeof(uint16_t),
           0);
   
   xdp_md_t ctx = {
       (uint64_t)packet_buffer,
       (uint64_t)packet_buffer + net_buffer->DataLength
   };

   if (ebpf_core_invoke_hook(EBPF_PROGRAM_TYPE_XDP, &ctx, &result) == EBPF_ERROR_SUCCESS)
   {
       switch (result)
       {
       case XDP_PASS:
           action = FWP_ACTION_PERMIT;
           break;
       case XDP_DROP:
           action = FWP_ACTION_BLOCK;
           break;
       }
   }
done:   
   classify_output->actionType = action;
   return;
}


static void
ebpf_hook_resource_allocation_classify(
    _In_ const FWPS_INCOMING_VALUES* incoming_fixed_values,
    _In_ const FWPS_INCOMING_METADATA_VALUES* incoming_metadata_values,
    _Inout_opt_ void* layer_data,
    _In_opt_ const void* classify_context,
    _In_ const FWPS_FILTER* filter,
    _In_ uint64_t flow_context,
    _Inout_ FWPS_CLASSIFY_OUT* classify_output)
/* ++

   A simple classify function at the WFP Resource Allocation layer.

-- */
{
    SOCKADDR_IN addr = {AF_INET};
    uint32_t result;
    bind_md_t ctx;
    
    UNREFERENCED_PARAMETER(layer_data);
    UNREFERENCED_PARAMETER(classify_context);
    UNREFERENCED_PARAMETER(filter);
    UNREFERENCED_PARAMETER(flow_context);

    addr.sin_port = incoming_fixed_values->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_IP_LOCAL_PORT].value.uint16;
    addr.sin_addr.S_un.S_addr = incoming_fixed_values->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_IP_LOCAL_ADDRESS].value.uint32;

    ctx.process_id = incoming_metadata_values->processId;
    memcpy(&ctx.socket_address, &addr, sizeof(addr));
    ctx.operation = BIND_OPERATION_BIND;
    ctx.protocol = incoming_fixed_values->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_IP_PROTOCOL].value.uint8;

    ctx.app_id_start = (uint64_t)incoming_fixed_values->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_ALE_APP_ID].value.byteBlob->data;
    ctx.app_id_end = ctx.app_id_start + incoming_fixed_values->incomingValue[FWPS_FIELD_ALE_RESOURCE_ASSIGNMENT_V4_ALE_APP_ID].value.byteBlob->size;

    if (ebpf_core_invoke_hook(
        EBPF_PROGRAM_TYPE_BIND,
        &ctx,
        &result) == EBPF_ERROR_SUCCESS)
    {
        switch (result)
        {
        case BIND_PERMIT:
        case BIND_REDIRECT:
            classify_output->actionType = FWP_ACTION_PERMIT;
            break;
        case BIND_DENY:
            classify_output->actionType = FWP_ACTION_BLOCK;

        }
    }

    return;
}

static void
ebpf_hook_resource_release_classify(
    _In_ const FWPS_INCOMING_VALUES* incoming_fixed_values,
    _In_ const FWPS_INCOMING_METADATA_VALUES* incoming_metadata_values,
    _Inout_opt_ void* layer_data,
    _In_opt_ const void* classify_context,
    _In_ const FWPS_FILTER* filter,
    _In_ uint64_t flow_context,
    _Inout_ FWPS_CLASSIFY_OUT* classify_output)
    /* ++

       A simple classify function at the WFP Resource Release layer.

    -- */
{
    SOCKADDR_IN addr = { AF_INET };
    uint32_t result;
    bind_md_t ctx;

    UNREFERENCED_PARAMETER(layer_data);
    UNREFERENCED_PARAMETER(classify_context);
    UNREFERENCED_PARAMETER(filter);
    UNREFERENCED_PARAMETER(flow_context);

    addr.sin_port = incoming_fixed_values->incomingValue[FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_IP_LOCAL_PORT].value.uint16;
    addr.sin_addr.S_un.S_addr = incoming_fixed_values->incomingValue[FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_IP_LOCAL_ADDRESS].value.uint32;

    ctx.process_id = incoming_metadata_values->processId;
    memcpy(&ctx.socket_address, &addr, sizeof(addr));
    ctx.operation = BIND_OPERATION_UNBIND;
    ctx.protocol = incoming_fixed_values->incomingValue[FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_IP_PROTOCOL].value.uint8;

    ctx.app_id_start = (uint64_t)incoming_fixed_values->incomingValue[FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_ALE_APP_ID].value.byteBlob->data;
    ctx.app_id_end = ctx.app_id_start + incoming_fixed_values->incomingValue[FWPS_FIELD_ALE_RESOURCE_RELEASE_V4_ALE_APP_ID].value.byteBlob->size;

    ebpf_core_invoke_hook(
        EBPF_PROGRAM_TYPE_BIND,
        &ctx,
        &result);

    classify_output->actionType = FWP_ACTION_PERMIT;

    return;
}

static NTSTATUS
ebpf_hook_no_op_notify(
    _In_ FWPS_CALLOUT_NOTIFY_TYPE callout_notification_type,
    _In_ const GUID* filter_key,
    _Inout_ const FWPS_FILTER* filter
)
{
    UNREFERENCED_PARAMETER(callout_notification_type);
    UNREFERENCED_PARAMETER(filter_key);
    UNREFERENCED_PARAMETER(filter);

    return STATUS_SUCCESS;
}

static void
ebpf_hook_no_op_flow_delete(
    _In_ uint16_t layer_id,
    _In_ uint32_t fwpm_callout_id,
    _In_ uint64_t flow_context
)
/* ++

   This is the flowDeleteFn function of the L2 callout.

-- */
{
    UNREFERENCED_PARAMETER(layer_id);
    UNREFERENCED_PARAMETER(fwpm_callout_id);
    UNREFERENCED_PARAMETER(flow_context);
    return;
}