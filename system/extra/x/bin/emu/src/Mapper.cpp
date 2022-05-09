#include "Mapper.h"
#include "MapperNROM.h"
#include "MapperSxROM.h"
#include "MapperUxROM.h"
#include "MapperCNROM.h"

namespace sn
{
    NameTableMirroring Mapper::getNameTableMirroring()
    {
        return static_cast<NameTableMirroring>(m_cartridge.getNameTableMirroring());
    }
   
    Mapper* Mapper::createMapper(Mapper::Type mapper_t, sn::Cartridge& cart, std::function<void(void)> mirroring_cb)
    {
		Mapper* ret = NULL;
        switch (mapper_t)
        {
            case NROM:
                ret = (new MapperNROM(cart));
                break;
            case SxROM:
                ret = (new MapperSxROM(cart, mirroring_cb));
                break;
            case UxROM:
                ret = (new MapperUxROM(cart));
                break;
            case CNROM:
                ret = (new MapperCNROM(cart));
                break;
            default:
                break;
        }
        return ret;
    }
}
