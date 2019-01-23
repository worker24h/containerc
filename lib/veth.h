


void veth_up(const char *veth_name);
void veth_config(const char *veth_name, char *ipv4);
void veth_create(const char *veth_name, const char *peer_veth_name);
int veth_ifindex(const char *veth_name);

