#include <stdio.h>
#include "Controller.h"

namespace sn
{
    Controller::Controller() :
        m_keyStates(0)
    {
        m_keyStorbe = 0;
        m_keyInput = 0;
	}

    void Controller::setKeyStatus(uint32_t key){
        m_keyInput = key;
    }

    void Controller::strobe(Byte b)
    {
        m_strobe = (b & 1);
        if (!m_strobe)
        {
            m_keyStorbe = m_keyInput;
        }
    }

    Byte Controller::read()
    {
        Byte ret = 0; 
        if(m_strobe) 
            return m_keyStorbe & 0x1;
        else{
            ret = (m_keyStorbe & 1);
            m_keyStorbe >>= 1; 
        }

        return ret | 0x40;
    }

}
