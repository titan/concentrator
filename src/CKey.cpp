#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include"CKey.h"
#include"CScreen.h"
#define KEY_NAME	"/dev/KBD0"

#ifdef DEBUG_KEY
#define DEBUG printf
#else
#define DEBUG(...)
#endif

CKey* CKey::m_Instance = NULL;
CLock CKey::m_Lock;
CKey* CKey::GetInstance()
{
   if( NULL == m_Instance )
   {
      m_Lock.Lock();
      if( NULL == m_Instance )
      {
         m_Instance = new CKey();
      }
      m_Lock.UnLock();
   }
   return m_Instance;
}

uint32 CKey::Run()
{
    //key
	fd_set rfds,fds;
	int retval;
	int fd;
	unsigned char number[1];

	fd = open(KEY_NAME,O_RDONLY);
	if (fd <0)
	{
		DEBUG("open device key error!\n");
		return 0;
	}

	FD_ZERO(&rfds);
	FD_ZERO(&fds);
	FD_SET(fd,&fds);

	//滤掉缓冲区
	while (read(fd,(char*)number, sizeof(unsigned char))>0);

	while(1)
	{
        struct timeval tv;
		tv.tv_sec=0;
		tv.tv_usec=10;
		rfds = fds;

		if (select(1+fd,&rfds,NULL,NULL,&tv)>0)
		{
			if(FD_ISSET(fd,&rfds))
			{
				retval=read(fd, (char*)number, sizeof(unsigned char));
				if (retval> 0)
                {
                    if( number[0] & 0x80 )//only respond to the up key
                    {
                        KeyE key = KEY_MAX;
                        switch(number[0]&0x7F)
                        {
                        case 0x28:
                            key = KEY_DOWN;
                            DEBUG("Key 'Down Arrow up'\n");
                            break;

                        case 0x26:
                            key = KEY_UP;
                            DEBUG("Key 'Up Arrow up'\n");
                            break;

                        case 0x25:
                            key = KEY_LEFT;
                            DEBUG("Key 'Left Arrow up'\n");
                            break;

                        case 0x27:
                            key = KEY_RIGHT;
                            DEBUG("Key 'Right Arrow up'\n");
                            break;

                        case 0x0D:
                            key = KEY_ENTER;
                            DEBUG("Key 'Enter up'\n");
                            break;

                        case 0x1B:
                            key = KEY_ESC;
                            DEBUG("Key 'Esc up'\n");
                            break;

                        default:
                            DEBUG("Key 'NULL up'\n");
                            break;
                        }
                        CScreen::GetInstance()->InputKey(key);
                    }
                }
			}
		}
	}
}
