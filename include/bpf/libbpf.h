// Copyright (c) Microsoft Corporation
// SPDX-License-Identifier: MIT

#ifdef __doxygen

/**
 * @name Link-related functions
 * @{
 */

/** @brief Clean up a link.
 *
 * @details Unless bpf_link__disconnect was called first,
 * this API detaches the link.  Either way, it then closes
 * the link fd and frees the memory for the link.
 *
 * @param[in] link Link to destroy.
 *
 * @sa bpf_link_detach
 * @sa ebpf_link_close
 */
int
bpf_link__destroy(struct bpf_link* link);

/** @brief Release ownership of a link so that it is
 * not detached when destroyed.
 *
 * @param[in] link Link to disconnect.
 *
 * @sa bpf_link__destroy
 */
void
bpf_link__disconnect(struct bpf_link* link);

/**
 * @brief Get a file descriptor that refers to a link.
 *
 * @param[in] link Link to get a file descriptor for.
 *
 * @returns File descriptor that refers to the link.
 */
int
bpf_link__fd(const struct bpf_link* link);

/**
 * @brief Pin a link to a specified path.
 *
 * @param[in] link Link to pin.
 * @param[in] path Path to pin the link to.
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 *
 * @exception EBUSY A pin path was previously specified.
 * @exception EEXIST Something is already pinned to the specified path.
 * @exception EINVAL An invalid argument was provided.
 * @exception ENOMEM Out of memory.
 *
 * @sa bpf_link__unpin
 */
int
bpf_link__pin(struct bpf_link* link, const char* path);

/**
 * @brief Unpin a link.
 *
 * @param[in] link Link to unpin.
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 *
 * @exception EINVAL An invalid argument was provided.
 * @exception ENOENT The link was not pinned.
 *
 * @sa bpf_link__pin
 */
int
bpf_link__unpin(struct bpf_link* link);

/** @} */

/**
 * @name Map-related functions
 * @{
 */

/**
 * @brief Get a file descriptor that refers to a map.
 *
 * @param[in] map Map to get a file descriptor for.
 *
 * @returns File descriptor that refers to the map.
 */
int
bpf_map__fd(const struct bpf_map* map);

/**
 * @brief Determine whether a map is pinned.
 *
 * @param[in] map Map to check.
 *
 * @retval true The map is pinned.
 * @retval false The map is not pinned.
 *
 * @sa bpf_map__pin
 * @sa bpf_object__pin
 * @sa bpf_object__pin_maps
 */
bool
bpf_map__is_pinned(const struct bpf_map* map);

/**
 * @brief Get the size of keys in a given map.
 *
 * @param[in] map Map to check.
 *
 * @returns The size in bytes of keys in the map.
 */
__u32
bpf_map__key_size(const struct bpf_map* map);

/**
 * @brief Get the maximum number of entries allowed in a given map.
 *
 * @param[in] map Map to check.
 *
 * @returns The maximum number of entries allowed.
 */
__u32
bpf_map__max_entries(const struct bpf_map* map);

/**
 * @brief Get the name of an eBPF map.
 *
 * @param[in] map The map to check.
 *
 * @returns The name of the map, or NULL if none.
 */
const char*
bpf_map__name(const struct bpf_map* map);

/**
 * @brief Get the next map for a given eBPF object.
 *
 * @param[in] map Previous map.
 * @param[in] obj Object with maps.
 *
 * @returns Next map, or NULL if none.
 *
 * @sa bpf_map__prev
 */
struct bpf_map*
bpf_map__next(const struct bpf_map* map, const struct bpf_object* obj);

/**
 * @brief Pin a map to a specified path.
 *
 * @param[in] map Map to pin.
 * @param[in] path Path to pin the map to.
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 *
 * @exception EBUSY A pin path was previously specified.
 * @exception EEXIST Something is already pinned to the specified path.
 * @exception EINVAL An invalid argument was provided.
 * @exception ENOMEM Out of memory.
 *
 * @sa bpf_map_unpin
 * @sa bpf_object__pin_maps
 */
int
bpf_map__pin(struct bpf_map* map, const char* path);

/**
 * @brief Get the previous map for a given eBPF object.
 *
 * @param[in] map Next map.
 * @param[in] obj Object with maps.
 *
 * @returns Previous map, or NULL if none.
 *
 * @sa bpf_map__next
 */
struct bpf_map*
bpf_map__prev(const struct bpf_map* map, const struct bpf_object* obj);

/**
 * @brief Get the type of a map.
 *
 * @param[in] map Map to check.
 *
 * @returns The map type.
 */
enum bpf_map_type
bpf_map__type(const struct bpf_map* map);

/**
 * @brief Unpin a map.
 *
 * @param[in] map Map to unpin.
 * @param[in] path Path from which to unpin the map.
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 *
 * @exception EINVAL An invalid argument was provided.
 * @exception ENOENT The map was not pinned.
 *
 * @sa bpf_map_pin
 * @sa bpf_object__unpin_maps
 */
int
bpf_map__unpin(struct bpf_map* map, const char* path);

/**
 * @brief Get the size of values in a given map.
 *
 * @param[in] map Map to check.
 *
 * @returns The size in bytes of values in the map.
 */
__u32
bpf_map__value_size(const struct bpf_map* map);

/** @} */

/**
 * @name Object-related functions
 * @{
 */

/**
 * @brief Close an eBPF object.
 *
 * @param[in] object The object to close.
 *
 * @sa bpf_prog_load
 */
void
bpf_object__close(struct bpf_object* object);

/**
 * @brief Find a map with a given name among maps associated with an eBPF object.
 *
 * @param[in] obj The object to check.
 * @param[in] name The name to look for.
 *
 * @returns The map found, or NULL if none.
 */
struct bpf_map*
bpf_object__find_map_by_name(const struct bpf_object* obj, const char* name);

/**
 * @brief Find a map with a given name among maps associated with an eBPF object.
 *
 * @param[in] obj The object to check.
 * @param[in] name The name to look for.
 *
 * @returns A file descriptor referring to the map found, or a negative value if none.
 *
 * @sa bpf_map__fd
 */
int
bpf_object__find_map_fd_by_name(const struct bpf_object* obj, const char* name);

/**
 * @brief Find a program with a given name among programs associated with an eBPF object.
 *
 * @param[in] obj The object to check.
 * @param[in] name The name to look for.
 *
 * @returns A file descriptor referring to the program found, or a negative value if none.
 *
 * @sa bpf_program__name
 */
struct bpf_program*
bpf_object__find_program_by_name(const struct bpf_object* obj, const char* name);

/**
 * @brief Load all the programs in a given object.
 *
 * @param[in] obj Object from which to load programs.
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 *
 * @exception EINVAL An invalid argument was provided.
 * @exception ENOMEM Out of memory.
 *
 * @sa bpf_object__load_xattr
 * @sa bpf_object__open
 * @sa bpf_object__unload
 * @sa bpf_prog_load
 */
int
bpf_object__load(struct bpf_object* obj);

/**
 * @brief Load all the programs in a given object.
 *
 * @param[in] attr Structure with load attributes.
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 *
 * @exception EINVAL An invalid argument was provided.
 * @exception ENOMEM Out of memory.
 *
 * @sa bpf_object__open
 * @sa bpf_object__load_xattr
 * @sa bpf_prog_load
 */
int
bpf_object__load_xattr(struct bpf_object_load_attr* attr);

/**
 * @brief Get the name of an eBPF object.
 *
 * @param[in] obj The object to check.
 *
 * @returns The name of the object, or NULL if none.
 */
const char*
bpf_object__name(const struct bpf_object* obj);

/**
 * @brief Get the next eBPF object opened by the calling process.
 *
 * @param[in] prev Previous object, or NULL to get the first object.
 *
 * @returns Next object, or NULL if none.
 */
struct bpf_object*
bpf_object__next(struct bpf_object* prev);

/**
 * @brief Open a file without loading the programs.
 *
 * @param[in] path File name to open.
 * @param[opts] opts Options to use when opening the object.
 *
 * @returns Pointer to an eBPF object, or NULL on failure.
 */
struct bpf_object*
bpf_object__open_file(const char* path, const struct bpf_object_open_opts* opts);

/**
 * @brief Pin an eBPF object to a specified path.
 *
 * @param[in] object Object to pin.
 * @param[in] path Path to pin the object to.
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 *
 * @exception EBUSY Something is already pinned to the specified path.
 * @exception EINVAL An invalid argument was provided.
 * @exception ENOMEM Out of memory.
 *
 * @sa bpf_object__pin_maps
 * @sa bpf_object__pin_programs
 * @sa bpf_object__unpin_maps
 * @sa bpf_object__unpin_programs
 */
int
bpf_object__pin(struct bpf_object* object, const char* path);

/**
 * @brief Pin all maps associated with an eBPF object to a specified path.
 *
 * @param[in] obj Object to pin maps of.
 * @param[in] path Path to pin the maps to.
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 *
 * @exception EBUSY Something is already pinned to the specified path.
 * @exception EINVAL An invalid argument was provided.
 * @exception ENOMEM Out of memory.
 *
 * @sa bpf_map__pin
 * @sa bpf_object__pin
 * @sa bpf_object__unpin_maps
 */
int
bpf_object__pin_maps(struct bpf_object* obj, const char* path);

/**
 * @brief Pin all programs associated with an eBPF object to a specified path.
 *
 * @param[in] obj Object to pin programs of.
 * @param[in] path Path to pin the programs to.
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 *
 * @exception EBUSY Something is already pinned to the specified path.
 * @exception EINVAL An invalid argument was provided.
 * @exception ENOMEM Out of memory.
 *
 * @sa bpf_program__pin
 * @sa bpf_object__pin
 * @sa bpf_object__unpin_programs
 */
int
bpf_object__pin_programs(struct bpf_object* obj, const char* path);

/**
 * @brief Unload all the programs in a given object.
 *
 * @param[in] obj Object with programs to be unloaded.
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 *
 * @sa bpf_object__load
 * @sa bpf_object__load_xattr
 * @sa bpf_prog_load
 */
int
bpf_object__unload(struct bpf_object* obj);

/**
 * @brief Unpin all maps associated with an eBPF object from a specified path.
 *
 * @param[in] obj Object to unpin maps of.
 * @param[in] path Path from which to unpin the maps.
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 *
 * @exception EINVAL An invalid argument was provided.
 *
 * @sa bpf_map__upnin
 * @sa bpf_object__pin_maps
 * @sa bpf_object__unpin
 */
int
bpf_object__unpin_maps(struct bpf_object* obj, const char* path);

/**
 * @brief Unpin all programs associated with an eBPF object from a specified path.
 *
 * @param[in] obj Object to unpin programs of.
 * @param[in] path Path from which to unpin the programs.
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 *
 * @exception EINVAL An invalid argument was provided.
 *
 * @sa bpf_program__unpin
 * @sa bpf_object__pin_mprograms
 * @sa bpf_object__unpin
 */
int
bpf_object__unpin_programs(struct bpf_object* obj, const char* path);

/** @} */

/**
 * @name Program-related functions
 * @{
 */

/*
 * @brief Load (but do not attach) an eBPF program from eBPF instructions
 * supplied by the caller.
 *
 * @param[in] type Program type to use.
 * @param[in] insns Array of eBPF instructions.
 * @param[in] insns_cnt Number of eBPF instructions in the array.
 * @param[in] license License.
 * @param[in] kern_version Kernel version.
 * @param[out] log_buf Buffer in which to write any log messages.
 * @param[in] log_buf_size Size in bytes of the log buffer.
 *
 * @returns File descriptor that refers to the program, or <0 on error.
 * The caller should call _close() on the fd to close this when done.
 *
 * @exception EACCES The program failed verification.
 * @exception EINVAL One or more parameters are incorrect.
 * @exception ENOMEM Out of memory.
 *
 * @sa bpf_prog_load
 * @sa bpf_load_program_xattr
 */
int
bpf_load_program(
    enum bpf_prog_type type,
    const struct bpf_insn* insns,
    size_t insns_cnt,
    const char* license,
    __u32 kern_version,
    char* log_buf,
    size_t log_buf_sz);

/*
 * @brief Load (but do not attach) an eBPF program from eBPF instructions
 * supplied by the caller.
 *
 * @param[in] load_attr Parameters to use to load the eBPF program.
 * @param[out] log_buf Buffer in which to write any log messages.
 * @param[in] log_buf_size Size in bytes of the log buffer.
 *
 * @returns File descriptor that refers to the program, or <0 on error.
 * The caller should call _close() on the fd to close this when done.
 *
 * @exception EACCES The program failed verification.
 * @exception EINVAL One or more parameters are incorrect.
 * @exception ENOMEM Out of memory.
 *
 * @sa bpf_prog_load
 * @sa bpf_load_program
 */
int
bpf_load_program_xattr(const struct bpf_load_program_attr* load_attr, char* log_buf, size_t log_buf_sz);

/**
 * @brief Load (but do not attach) eBPF maps and programs from an ELF file.
 *
 * @param[in] file Path name to an ELF file.
 * @param[in] type Program type to use for loading eBPF programs.  If BPF_PROG_TYPE_UNKNOWN,
 * the program type is derived from the section prefix in the ELF file.
 * @param[out] pobj Pointer to where to store the eBPF object loaded. The caller
 * is expected to call bpf_object__close() to free the object.
 * @param[out] prog_fd Returns a file descriptor for the first program.
 * The caller should not call _close() on the fd, but should instead use
 * bpf_object__close() on the object returned.
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 *
 * @exception EACCES The program failed verification.
 * @exception EINVAL One or more parameters are incorrect.
 * @exception ENOMEM Out of memory.
 *
 * @sa bpf_load_program
 * @sa bpf_load_program_xattr
 * @sa bpf_object__close
 * @sa bpf_program__attach
 */
int
bpf_prog_load(const char* file, enum bpf_prog_type type, struct bpf_object** pobj, int* prog_fd);

/**
 * @brief Attach an eBPF program to a hook associated with the program's expected attach type.
 *
 * @param[in] prog The program to attach.
 *
 * @returns The link created.  On error, returns NULL and sets errno.
 *
 * @sa bpf_link__destroy
 * @sa bpf_program__get_expected_attach_type
 * @sa bpf_program__set_expected_attach_type
 * @sa ebpf_link_close
 */
struct bpf_link*
bpf_program__attach(const struct bpf_program* prog);

/**
 * @brief Attach an eBPF program to an XDP hook.
 *
 * @param[in] prog The program to attach.
 * @param[in] ifindex The interface index to attach to.
 *
 * @returns The link created.  On error, returns NULL and sets errno.
 *
 * @sa bpf_link__destroy
 * @sa bpf_program__attach
 * @sa ebpf_link_close
 */
struct bpf_link*
bpf_program__attach_xdp(struct bpf_program* prog, int ifindex);

/**
 * @brief Get a file descriptor that refers to a program.
 *
 * @param[in] prog Program to get a file descriptor for.
 *
 * @returns File descriptor that refers to the program.
 */
int
bpf_program__fd(const struct bpf_program* prog);

/**
 * @brief Get the expected attach type for an eBPF program.
 *
 * @param[in] prog Program to check.
 *
 * @returns Expected attach type.
 *
 * @sa bpf_program__attach
 * @sa bpf_program__set_expected_attach_type
 */
enum bpf_attach_type
bpf_program__get_expected_attach_type(const struct bpf_program* prog);

/**
 * @brief Get the program type for an eBPF program.
 *
 * @param[in] prog Program to check.
 *
 * @returns Program type.
 *
 * @sa bpf_program__get_expected_attach_type
 */
enum bpf_prog_type
bpf_program__get_type(const struct bpf_program* prog);

/**
 * @brief Get the function name of an eBPF program.
 *
 * @param[in] prog Program to check.
 *
 * @returns The name of the program, which is the name of the main
 * function called when invoked.
 *
 * @sa bpf_object__find_program_by_name
 */
const char*
bpf_program__name(const struct bpf_program* prog);

/**
 * @brief Get the next program for a given eBPF object.
 *
 * @param[in] prog Previous program, or NULL to get the first program.
 * @param[in] obj Object with programs.
 *
 * @returns Next program, or NULL if none.
 *
 * @sa bpf_program__prev
 */
struct bpf_program*
bpf_program__next(struct bpf_program* prog, const struct bpf_object* obj);

/**
 * @brief Pin a program to a specified path.
 *
 * @param[in] prog Program to pin.
 * @param[in] path Path to pin the program to.
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 *
 * @exception EBUSY A pin path was previously specified.
 * @exception EEXIST Something is already pinned to the specified path.
 * @exception EINVAL An invalid argument was provided.
 * @exception ENOMEM Out of memory.
 *
 * @sa bpf_object__pin
 * @sa bpf_object__pin_programs
 * @sa bpf_program__unpin
 */
int
bpf_program__pin(struct bpf_program* prog, const char* path);

/**
 * @brief Get the previous eBPF program for a given eBPF object.
 *
 * @param[in] prog Next program.
 * @param[in] obj Object with programs.
 *
 * @returns Previous eBPF program, or NULL if none.
 *
 * @sa bpf_program__next
 */
struct bpf_program*
bpf_program__prev(struct bpf_program* prog, const struct bpf_object* obj);

/**
 * @brief Gets the ELF section name of an eBPF program, if any.
 *
 * @param[in] prog An eBPF program.
 *
 * @returns The ELF section name of an eBPF program, or NULL if none.
 */
const char*
bpf_program__section_name(const struct bpf_program* prog);

/**
 * @brief Set the expected attach type for an eBPF program.
 *
 * @param[in] prog Program to update.
 * @param[in] type Attach type to set.
 *
 * @sa bpf_program__attach
 * @sa bpf_program__get_expected_attach_type
 */
void
bpf_program__set_expected_attach_type(struct bpf_program* prog, enum bpf_attach_type type);

/**
 * @brief Set the program type for an eBPF program.
 *
 * @param[in] prog Program to update.
 * @param[in] type Program type to set.
 *
 * @sa bpf_program__set_expected_attach_type
 */
void
bpf_program__set_type(struct bpf_program* prog, enum bpf_prog_type type);

/**
 * @brief Unload a program.
 *
 * @param[in] prog Program to unload.
 *
 * @sa bpf_object__unload
 * @sa bpf_prog_load
 */
void
bpf_program__unload(struct bpf_program* prog);

/**
 * @brief Unpin a program.
 *
 * @param[in] prog Program to unpin.
 * @param[in] path Path from which to unpin the program.
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 *
 * @exception EINVAL An invalid argument was provided.
 * @exception ENOENT The program was not pinned.
 *
 * @sa bpf_object__unpin_programs
 * @sa bpf_program__pin
 */
int
bpf_program__unpin(struct bpf_program* prog, const char* path);

/**
 * @brief Attach an XDP program to a given interface.
 *
 * @param[in] ifindex The interface index to attach to, or -1 to detach.
 * @param[in] fd File descriptor of program to attach.
 * @param[in] flags Flags. Use XDP_FLAGS_REPLACE to replace any program previously attached to
 *                  the specified interface index.
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 *
 * @deprecated Use bpf_xdp_attach() instead.
 *
 * @sa bpf_program__attach_xdp
 * @sa bpf_xdp_attach
 * @sa bpf_xdp_detach
 */
int
bpf_set_link_xdp_fd(int ifindex, int fd, __u32 flags);

/**
 * @brief Attach an XDP program to a given interface.
 *
 * @param[in] ifindex The interface index to attach to, or -1 to detach.
 * @param[in] prog_fd File descriptor of program to attach.
 * @param[in] flags Flags. Use XDP_FLAGS_REPLACE to replace any program previously attached to
 *                  the specified interface index.
 * @param[in] opts Options (currently unused).
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 *
 * @sa bpf_program__attach_xdp
 * @sa bpf_xdp_detach
 */
int
bpf_xdp_attach(int ifindex, int prog_fd, __u32 flags, const struct bpf_xdp_attach_opts* opts);

/**
 * @brief Detach an XDP program from a given interface.
 *
 * @param[in] ifindex The interface index to detach from.
 * @param[in] prog_fd File descriptor of program to detach.
 * @param[in] flags Flags. Use XDP_FLAGS_REPLACE to detach any program previously attached to
 *                  the specified interface index.
 * @param[in] opts Options (currently unused).
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 *
 * @sa bpf_link_detach
 * @sa bpf_program__attach_xdp
 * @sa bpf_xdp_attach
 */
int
bpf_xdp_detach(int ifindex, __u32 flags, const struct bpf_xdp_attach_opts* opts);

/**
 * @brief Get the XDP program id attached to a given interface index.
 *
 * @param[in] ifindex The interface index to query.
 * @param[in] flags Flags (currently 0).
 * @param[out] prog_id The ID of the program attached.
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 *
 * @sa bpf_xdp_attach
 */
int
bpf_xdp_query_id(int ifindex, int flags, __u32* prog_id);

/**
 * @brief Get a program type and expected attach type by name.
 *
 * @param[in] name Name, as if it were a section name in an ELF file.
 * @param[out] prog_type Program type.
 * @param[out] expected_attach_type Expected attach type.
 *
 * @retval 0 The operation was successful.
 * @retval <0 An error occured, and errno was set.
 */
int
libbpf_prog_type_by_name(const char* name, enum bpf_prog_type* prog_type, enum bpf_attach_type* expected_attach_type);

/** @} */

/**
 * @name System-related functions
 * @{
 */

/**
 * @brief Get a negative error code based on errno and a possibly null pointer.
 *
 * @param[in] ptr Pointer that may be NULL.
 *
 * @returns Negative error code.
 */
long
libbpf_get_error(const void* ptr);

/**
 * @brief Get the number of processors on the current system.
 *
 * @returns Number of processors.
 */
int
libbpf_num_possible_cpus(void);

/* Ring buffer APIs */

/**
 * @brief Creates a new ring buffer manager.
 *
 * @param[in] map_fd File descriptor to ring buffer map.
 * @param[in] sample_cb Pointer to ring buffer notification callback function.
 * @param[in] ctx Pointer to sample_cb callback function.
 * @param[in] opts Ring buffer options.
 *
 * @returns Pointer to ring buffer manager.
 */
struct ring_buffer*
ring_buffer__new(int map_fd, ring_buffer_sample_fn sample_cb, void* ctx, const struct ring_buffer_opts* opts);

/**
 * @brief Frees a new ring buffer manager.
 *
 * @param[in] rb Pointer to ring buffer to be freed.
 *
 */
void
ring_buffer__free(struct ring_buffer* rb);
/** @} */

#else
#pragma warning(push)
#pragma warning(disable : 4200) // Zero-sized array in struct/union
#pragma warning(disable : 4201) // Zero-sized array in struct/union
#include "../external/bpftool/libbpf/src/libbpf.h"
#pragma warning(pop)
#endif
