/*
 *  DvoracleKeyboardHandler.cpp
 *  Dvoracle
 *
 *
 */

#include "DvoracleKeyboardHandler.h"
#include "DvoracleRemapper.h"
#include "KeyConstants.h"

namespace com_phogy_Dvoracle
{

    DvoracleKeyboardHandler *DvoracleKeyboardHandler::s_pInstance = 0;

    // Event handlers for the keyboard
    void event  (                          OSObject * target,
                    /* eventFlags  */      unsigned   eventType,
                    /* flags */            unsigned   flags,
                    /* keyCode */          unsigned   key,
                    /* charCode */         unsigned   charCode,
                    /* charSet */          unsigned   charSet,
                    /* originalCharCode */ unsigned   origCharCode,
                    /* originalCharSet */  unsigned   origCharSet,
                    /* keyboardType */     unsigned   keyboardType,
                    /* repeat */           bool       repeat,
                    /* atTime */           AbsoluteTime ts);
                    
                    
	DvoracleKeyboardHandler::DvoracleKeyboardHandler()
	{
        //IOLog("Construct DvoracleKeyboardHandler\n");
	}
	
	DvoracleKeyboardHandler::~DvoracleKeyboardHandler()
	{
        //IOLog("Destruct DvoracleKeyboardHandler\n");
        s_pInstance = 0;
	}
	
	bool DvoracleKeyboardHandler::IsValidKeyboardType(const char *pTypeName)
	{
		bool ret = false;
		
		if (strcmp(pTypeName, "AppleADBKeyboard") == 0)
		{
			ret = true;
		}
		else if (strcmp(pTypeName, "AppleADBButtons") == 0)
		{
			// Not supported
		}
		else if (strcmp(pTypeName, "IOHIDKeyboard") == 0)
		{
			ret = true;
		}
		else if (strcmp(pTypeName, "IOHIDConsumer") == 0)
		{
			// Not supported
		}
		else if (strcmp(pTypeName, "ApplePS2Keyboard") == 0)
		{
			ret = true;
		}
		
		return ret;
	}
    
    
    DvoracleKeyboardHandler::KeybEntry *DvoracleKeyboardHandler::FindCurrentKeybEntry()
    {
        for (int i = 0; i < MAX_KEYBOARDS; i++)
        {
            if (m_keyboards[i].pKbd->_keyboardEventAction == event)
            {
                return &m_keyboards[i];
            }
        }
        
        return 0;
    }
	
	void DvoracleKeyboardHandler::StartKeyboard(IOHIKeyboard *pKbd)
	{
		if (pKbd && IsValidKeyboardType(pKbd->getName()))
		{
			IOLog("Dvoracle found keyboard: %s\n", pKbd->getName());
			
            for (int i = 0; i < MAX_KEYBOARDS; i++)
            {
                if (!m_keyboards[i].pKbd)
                {
                    // We found a free slot
                    m_keyboards[i].pKbd = pKbd;
                    m_keyboards[i].originalEventAction = pKbd->_keyboardEventAction;
                    //m_keyboards[i].originalSpecialEventAction = pKbd->_keyboardSpecialEventAction;
                    
                    // Patch ourselves in between
                    pKbd->_keyboardEventAction = event;
                    
                    //IOLog("Keyboard event action hooked at %d (orig=%08X new=%08X) pKbd = %08X\n",
                    //    i, m_keyboards[i].originalEventAction,
                    //    pKbd->_keyboardEventAction, m_keyboards[i].pKbd);
                    break;
                } 
                else if (m_keyboards[i].pKbd == pKbd)
                {
                    IOLog("Dvoracle WARNING: Keyboard appears again: %s\n", pKbd->getName());
                    break;
                }
            }
		}
		else
		{
			//IOLog("Dvoracle ERROR: Unsupported Keyboard type: %s\n", pKbd ? pKbd->getName() : "(None)");
		}
	}
	
	
	void DvoracleKeyboardHandler::StopKeyboard(IOHIKeyboard *pKbd)
	{
        IOLog("StopKeyboard+++\n");
        for (int i = 0; i < MAX_KEYBOARDS; i++)
        {
            if (pKbd && m_keyboards[i].pKbd == pKbd)
            {
                
                pKbd->_keyboardEventAction = m_keyboards[i].originalEventAction;
                IOLog("Dvoracle Stopped keyboard type: %s (%08X)\n", pKbd->getName(),
                    pKbd->_keyboardEventAction); 
                return;
            }
        } 
	
        IOLog("Dvoracle ERROR: Cannot find the keyboard to stop: %s\n", pKbd ? pKbd->getName() : "(None)");
	}
    
    void DvoracleKeyboardHandler::StopAllKeyboards()
    {
        for (int i = 0; i < MAX_KEYBOARDS; i++)
        {
            if (m_keyboards[i].pKbd)
            {
                m_keyboards[i].pKbd->_keyboardEventAction = m_keyboards[i].originalEventAction;
                IOLog("Dvoracle Stopped keyboard type: %s (%08X)\n", m_keyboards[i].pKbd->getName(),
                    m_keyboards[i].pKbd->_keyboardEventAction); 
            }
        }
    }
    
    void DvoracleKeyboardHandler::HandleEvent(KeybEvent& event)
    {
        //IOLog("HandleEvent+++\n");
        KeybEntry *pCurrentKeyb = FindCurrentKeybEntry();
        
        if (pCurrentKeyb)
        {
            bool isMoreInSequence;
            unsigned sequenceIndex = 0;
            
            do
            {
                KeybEvent oldEvent = event;

                isMoreInSequence = false;
                DvoracleRemapper::GetInstance()->Remap(event, isMoreInSequence, sequenceIndex++);
                
                if (event.eventType != KEY_CANCEL)
                {
                    pCurrentKeyb->originalEventAction(event.target, 
                        event.eventType,
                        event.flags,
                        event.key,
                        event.charCode,
                        event.charSet,
                        event.origCharCode,
                        event.origCharSet,
                        event.keyboardType,
                        event.repeat,
                        event.ts);
                        
                }
                
                if (isMoreInSequence)
                {
                    event = oldEvent;
                }
            }
            while (isMoreInSequence && sequenceIndex < MAX_SEQUENCE);
        }
        //IOLog("HandleEvent---\n");
        
    }


    void event  (                          OSObject * target,
                    /* eventFlags  */      unsigned   eventType,
                    /* flags */            unsigned   flags,
                    /* keyCode */          unsigned   key,
                    /* charCode */         unsigned   charCode,
                    /* charSet */          unsigned   charSet,
                    /* originalCharCode */ unsigned   origCharCode,
                    /* originalCharSet */  unsigned   origCharSet,
                    /* keyboardType */     unsigned   keyboardType,
                    /* repeat */           bool       repeat,
                    /* atTime */           AbsoluteTime ts)
    {
        //IOLog("event+++\n");
        KeybEvent keybEvent;
        keybEvent.target = target;
        keybEvent.eventType = eventType;
        keybEvent.flags = flags;
        keybEvent.key = key;
        keybEvent.charCode = charCode;
        keybEvent.charSet = charSet;
        keybEvent.origCharCode = origCharCode;
        keybEvent.origCharSet = origCharSet;
        keybEvent.keyboardType = keyboardType;
        keybEvent.repeat = repeat;
        keybEvent.ts = ts;

        DvoracleKeyboardHandler::GetInstance()->HandleEvent(keybEvent);
        //IOLog("event---\n");
    }
}
