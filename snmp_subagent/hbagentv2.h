#ifndef _HBAGENTV2_H 
#define _HBAGENTV2_H 



struct hb_rsinfov2 {
    size_t index;
    char * resourceid;
    uint32_t type;
    uint32_t status;
    char * node;
    uint32_t is_managed;
    uint32_t failcount;
    char * parent;
};

#endif  /* _HBAGENTV2_H */
