/* lab/157-tls: the general-dynamic model, in one function.
 *
 * A shared library cannot know the TLS offset at link time (the final
 * program lays out the template), so it lowers to: load an index,
 * call __tls_get_addr, use the returned address. Build with -fPIC -shared
 * and read the disassembly: lea + call __tls_get_addr@plt.
 */
__thread int shared_counter;

int bump(void) {
    return ++shared_counter;
}
