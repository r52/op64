/**********************************************************************************
Common Audio plugin spec, version #1.1 maintained by zilmar (zilmar@emulation64.com)

All questions or suggestions should go through the mailing list.
http://www.egroups.com/group/Plugin64-Dev
**********************************************************************************
Notes:
------

Setting the approprate bits in the MI_INTR_REG and calling CheckInterrupts which 
are both passed to the DLL in InitiateAudio will generate an Interrupt from with in 
the plugin.

**********************************************************************************/
#ifndef _AUDIO_H_INCLUDED__
#define _AUDIO_H_INCLUDED__

#if defined(__cplusplus)
extern "C" {
#endif

#define PLUGIN_TYPE_AUDIO			3

#if defined(WIN32)
#define EXPORT      __declspec(dllexport)
#define CALL        __cdecl
#else
#define EXPORT      __attribute__((visibility("default")))
#define CALL
#endif

#define SYSTEM_NTSC					0
#define SYSTEM_PAL					1
#define SYSTEM_MPAL					2

/***** Structures *****/
typedef struct {
	uint16_t Version;
	uint16_t Type;
	char Name[100];

	int NormalMemory;
	int MemoryBswaped;
} PLUGIN_INFO;


typedef struct {
	void* hwnd;
	void* hinst;
	int MemoryBswaped;

	uint8_t* HEADER;
	uint8_t* RDRAM;
	uint8_t* DMEM;
	uint8_t* IMEM;

	uint32_t* MI_INTR_REG;

	uint32_t* AI_DRAM_ADDR_REG;
	uint32_t* AI_LEN_REG;
	uint32_t* AI_CONTROL_REG;
	uint32_t* AI_STATUS_REG;
	uint32_t* AI_DACRATE_REG;
	uint32_t* AI_BITRATE_REG;

	void (*CheckInterrupts)( void );
} AUDIO_INFO;

/******************************************************************
  Function: AiDacrateChanged
  Purpose:  This function is called to notify the dll that the
            AiDacrate registers value has been changed.
  input:    The System type:
               SYSTEM_NTSC	0
               SYSTEM_PAL	1
               SYSTEM_MPAL	2
  output:   none
*******************************************************************/ 
EXPORT void CALL AiDacrateChanged (int SystemType);

/******************************************************************
  Function: AiLenChanged
  Purpose:  This function is called to notify the dll that the
            AiLen registers value has been changed.
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL AiLenChanged (void);

/******************************************************************
  Function: AiReadLength
  Purpose:  This function is called to allow the dll to return the
            value that AI_LEN_REG should equal
  input:    none
  output:   The amount of bytes still left to play.
*******************************************************************/ 
EXPORT uint32_t CALL AiReadLength (void);

/******************************************************************
  Function: AiUpdate
  Purpose:  This function is called to allow the dll to update
            things on a regular basis (check how long to sound to
			go, copy more stuff to the buffer, anyhting you like).
			The function is designed to go in to the message loop
			of the main window ... but can be placed anywhere you 
			like.
  input:    if Wait is set to true, then this function should wait
            till there is a messgae in the its message queue.
  output:   none
*******************************************************************/ 
//EXPORT void CALL AiUpdate (int Wait);

/******************************************************************
  Function: CloseDLL
  Purpose:  This function is called when the emulator is closing
            down allowing the dll to de-initialise.
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL CloseDLL (void);

/******************************************************************
  Function: DllAbout
  Purpose:  This function is optional function that is provided
            to give further information about the DLL.
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/ 
EXPORT void CALL DllAbout ( void* hParent );

/******************************************************************
  Function: DllConfig
  Purpose:  This function is optional function that is provided
            to allow the user to configure the dll
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/ 
EXPORT void CALL DllConfig ( void* hParent );

/******************************************************************
  Function: DllTest
  Purpose:  This function is optional function that is provided
            to allow the user to test the dll
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/ 
EXPORT void CALL DllTest ( void* hParent );

/******************************************************************
  Function: GetDllInfo
  Purpose:  This function allows the emulator to gather information
            about the dll by filling in the PluginInfo structure.
  input:    a pointer to a PLUGIN_INFO stucture that needs to be
            filled by the function. (see def above)
  output:   none
*******************************************************************/ 
EXPORT void CALL GetDllInfo ( PLUGIN_INFO* PluginInfo );

/******************************************************************
  Function: InitiateSound
  Purpose:  This function is called when the DLL is started to give
            information from the emulator that the n64 audio 
			interface needs
  Input:    Audio_Info is passed to this function which is defined
            above.
  Output:   TRUE on success
            FALSE on failure to initialise
             
  ** note on interrupts **:
  To generate an interrupt set the appropriate bit in MI_INTR_REG
  and then call the function CheckInterrupts to tell the emulator
  that there is a waiting interrupt.
*******************************************************************/ 
EXPORT int CALL InitiateAudio (AUDIO_INFO Audio_Info);

/******************************************************************
  Function: ProcessAList
  Purpose:  This function is called when there is a Alist to be
            processed. The Dll will have to work out all the info
			about the AList itself.
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL ProcessAList(void);

/******************************************************************
  Function: RomClosed
  Purpose:  This function is called when a rom is closed.
  input:    none
  output:   none
*******************************************************************/ 
EXPORT void CALL RomClosed (void);

#if defined(__cplusplus)
}
#endif

#endif
