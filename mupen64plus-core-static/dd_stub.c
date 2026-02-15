/* dd_stub.c - Stub implementations for 64DD functions */
#include <stddef.h>

unsigned int get_zone_from_head_track(unsigned int head, unsigned int track) { return 0; }
unsigned int get_sector_base(unsigned int zone) { return 0; }
const char* get_disk_format_name(int format) { return "Unknown"; }
void GenerateLBAToPhysTable(void* dd, void* lba_table) { }
int scan_and_expand_disk_format(void* data, unsigned int size) { return 0; }

/* Storage interface stubs */
void* g_istorage_disk_full = NULL;
void* g_istorage_disk_ram_only = NULL;  
void* g_istorage_disk_read_only = NULL;
