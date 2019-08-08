#include "tb_mem_test.h"

//-----------------------------------------------------------------
// get_mem_address: Get a random address, with enough space
//-----------------------------------------------------------------
uint32_t tb_mem_test::get_mem_address(int space, int alignment)
{
    bool found = false;
    uint32_t addr = 0;
    int num_regions = 0;

    for (int i=0;i<TB_MEM_MAX_REGIONS;i++)
        if (m_mem[i])
            num_regions++;

    do
    {
        int i = rand() % num_regions;

        uint32_t base = m_mem[i]->get_base();
        uint32_t size = m_mem[i]->get_size();

        if (space < size)
        {
            size -= space;
            addr  = base + (rand() % size) & ~(alignment - 1);
            sc_assert(addr >= base);
            sc_assert((addr - base + space) < m_mem[i]->get_size());
            found = true;
            break;
        }
    }
    while (!found);

    return addr;
}
//-----------------------------------------------------------------
// process: Random reads and writes
//-----------------------------------------------------------------
void tb_mem_test::process(void)
{
    while (true)
    {
        m_enabled.wait();
        printf("Starting memory test sequence...\n");

        int iterations = m_iterations.read();

        while ((iterations == -1) || (iterations-- >= 1))
        {
            switch (rand() % 4)
            {
                // Word write
                case 0:
                {
                    uint32_t addr = get_mem_address(4, 4);

                    sc_uint <32> data;
                    data.range(31, 24) = rand();
                    data.range(23, 16) = rand();
                    data.range(15, 8)  = rand();
                    data.range(7, 0)   = rand();

                    for (int i=0;i<4;i++)
                        this->write(addr + i, (uint8_t)data.range((i*8)+7,(i*8)));

                    m_driver->write32(addr, data);
                }
                break;
                // Word read
                case 1:
                {
                    uint32_t addr = get_mem_address(4, 4);

                    sc_uint <32> data    = 0; 
                    for (int i=0;i<4;i++)
                        data.range((i*8)+7,(i*8)) = this->read(addr+i);

                    sc_uint <32> data_rd = m_driver->read32(addr);
                    if (data_rd != data)
                        printf("MISMATCH: %08x -> %02x != %02x\n", addr, (uint32_t)data_rd, (uint32_t)data);
                    sc_assert(data_rd == data);
                }
                break;
                // Block read
                case 2:
                {
                    int    length = 1 + (rand() % m_max_length);
                    uint32_t addr = get_mem_address(length, 1);
                    uint8_t *buffer = new uint8_t[length];
                    
                    m_driver->read(addr, buffer, length);

                    for (int i=0;i<length;i++)
                    {
                        if (this->read(addr + i) != buffer[i])
                            printf("MISMATCH: %08x -> %02x != %02x\n", addr + i, buffer[i], this->read(addr + i));
                        sc_assert(this->read(addr + i) == buffer[i]);
                    }

                    delete buffer;
                    buffer = NULL;
                }
                break;
                // Block write
                case 3:
                {
                    int    length = 1 + (rand() % m_max_length);
                    uint32_t addr = get_mem_address(length, 1);
                    uint8_t *buffer = new uint8_t[length];

                    for (int i=0;i<length;i++)
                    {
                        buffer[i] = rand();
                        this->write(addr + i, buffer[i]);
                    }

                    m_driver->write(addr, buffer, length);

                    delete buffer;
                    buffer = NULL;
                }
                break;
            }
        }

        // Notify completion
        printf("Completed memory test sequence...\n");
        m_completed.post();
    }
}