// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#include "ebpf_core_structs.h"

#pragma once
namespace Platform {
bool
DeviceIoControl(
    _In_ ebpf_handle_t device_handle,
    uint32_t io_control_code,
    _In_reads_bytes_opt_(input_buffer_size) void* input_buffer,
    uint32_t input_buffer_size,
    _Out_writes_bytes_to_opt_(output_buffer_size, *count_of_bytes_returned) void* output_buffer,
    uint32_t output_buffer_size,
    _Out_opt_ uint32_t* count_of_bytes_returned,
    _Inout_opt_ OVERLAPPED* overlapped);

bool
CancelIoEx(_In_ ebpf_handle_t device_handle, _In_opt_ OVERLAPPED* overlapped);

ebpf_handle_t
CreateFileW(
    _In_ PCWSTR file_name,
    uint32_t desired_access,
    uint32_t share_mode,
    _In_opt_ SECURITY_ATTRIBUTES* security_attributes,
    uint32_t creation_disposition,
    uint32_t flags_and_attributes,
    _In_opt_ ebpf_handle_t template_file);

bool
CloseHandle(_In_ _Post_ptr_invalid_ ebpf_handle_t handle);

bool
DuplicateHandle(
    _In_ ebpf_handle_t source_process_handle,
    _In_ ebpf_handle_t source_handle,
    _In_ ebpf_handle_t target_process_handle,
    _Out_ ebpf_handle_t* target_handle,
    uint32_t desired_access,
    bool inherit_handle,
    uint32_t options);

int
_open_osfhandle(intptr_t os_file_handle, int flags);

intptr_t
_get_osfhandle(int file_descriptor);

int
_close(int file_descriptor);

} // namespace Platform
