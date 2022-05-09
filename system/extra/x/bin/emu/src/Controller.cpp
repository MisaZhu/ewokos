#include "Controller.h"

namespace sn
{
    Controller::Controller() :
        m_keyStates(0)
    {
	}

    void Controller::strobe(Byte b)
    {
        m_strobe = (b & 1);
        if (!m_strobe)
        {
            m_keyStates = 0;
            int shift = 0;
            for (int button = A; button < TotalButtons; ++button)
            {
                ++shift;
            }
        }
    }

    Byte Controller::read()
    {
        Byte ret = 0; 
        return ret | 0x40;
    }

}
