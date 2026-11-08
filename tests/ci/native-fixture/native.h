typedef enum ProbeState { PROBE_READY = 7 } ProbeState;
ProbeState inspect_name(const char *name);
typedef struct ProbeBuffer ProbeBuffer;
ProbeBuffer *probe_open(void);
void probe_close(ProbeBuffer *buffer);
int probe_length(ProbeBuffer *buffer);
const unsigned char *probe_bytes(ProbeBuffer *buffer);
