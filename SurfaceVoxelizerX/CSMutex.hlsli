//--------------------------------------------------------------------------------------
// By XU, Tianchen
//--------------------------------------------------------------------------------------

#ifndef U_MUTEX
#define	U_MUTEX			u4
#endif

#define	mutexLock(x)	{	\
							uint uLock;				\
							[allow_uav_condition]	\
							for (uint i = 0; i < 0xffffffff; ++i)	\
							{	\
								InterlockedExchange(g_RWMutex[x], 1, uLock);	\
								DeviceMemoryBarrier();							\
								if (uLock != 1)	\
								{
#define	mutexUnlock(x)				g_RWMutex[x] = 0;	\
									break;				\
								}	\
							}	\
						}

RWTexture3D<uint>		g_RWMutex	: register (U_MUTEX);
