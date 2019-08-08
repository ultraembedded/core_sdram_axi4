#ifndef TB_DRIVER_API_H
#define TB_DRIVER_API_H

#include <stdio.h>
#include <unistd.h>

//-------------------------------------------------------------
// tb_driver_api: Base class for drivers
//-------------------------------------------------------------
class tb_driver_api
{
public:
    virtual void      write32(uint32_t addr, uint32_t data) = 0;
    virtual uint32_t  read32(uint32_t addr) = 0;
    virtual void      write(uint32_t addr, uint8_t *data, int length) = 0;
    virtual void      read(uint32_t addr, uint8_t *data, int length) = 0;
};

#endif