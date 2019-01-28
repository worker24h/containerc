
#ifndef VETH_H
#define VETH_H
void veth_up(const char *veth_name);
void veth_config_ipv4(const char *veth_name, char *ipv4);
void veth_create(const char *veth_name, const char *peer_veth_name);
int veth_ifindex(const char *veth_name);
void veth_network_namespace(const char* veth_name, int child_pid);
void veth_newname(const char *veth_name, const char* newname);
void veth_addbr(const char *veth_name, const char* brname);
#endif

