#ifndef PODMAN_H
#define PODMAN_H

void podman_init(void);
char const *podman_exe(void);
int podman_stop(char const *name, int secs, void (*callb)(int, void *),
                void *arg);
int podman_rm(char const *name, void (*callb)(int, void *), void *arg);

#endif
