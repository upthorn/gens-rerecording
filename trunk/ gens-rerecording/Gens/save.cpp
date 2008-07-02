#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include "save.h"
#include "cpu_68k.h"
#include "cpu_sh2.h"
#include "sh2.h"
#include "z80.h"
#include "cd_aspi.h"
#include "gens.h"
#include "G_main.h"
#include "G_ddraw.h"
#include "G_dsound.h"
#include "G_input.h"
#include "gfx_cd.h"
#include "vdp_io.h"
#include "vdp_rend.h"
#include "vdp_32X.h"
#include "rom.h"
#include "mem_M68K.h"
#include "mem_S68K.h"
#include "mem_SH2.h"
#include "mem_Z80.h"
#include "ym2612.h"
#include "psg.h"
#include "pcm.h"
#include "pwm.h"
#include "lc89510.h"
#include "scrshot.h"
#include "ggenie.h"
#include "io.h"
#include "misc.h"
#include "cd_sys.h"
#include "movie.h"

int Current_State = 0;
char State_Dir[1024] = "";
char SRAM_Dir[1024] = "";
char BRAM_Dir[1024] = "";
unsigned short FrameBuffer[336 * 240];
unsigned int FrameBuffer32[336 * 240];
unsigned char State_Buffer[MAX_STATE_FILE_LENGTH];
bool UseMovieStates;
extern long x, y, xg, yg;

int Change_File_S(char *Dest, char *Dir, char *Titre, char *Filter, char *Ext)
{
	OPENFILENAME ofn;

	SetCurrentDirectory(Gens_Path);

	if (!strcmp(Dest, ""))
	{
		strcpy(Dest, "default.");
		strcat(Dest, Ext);
	}

	memset(&ofn, 0, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = HWnd;
	ofn.hInstance = ghInstance;
	ofn.lpstrFile = Dest;
	ofn.nMaxFile = 2047;
	ofn.lpstrFilter = Filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrInitialDir = Dir;
	ofn.lpstrTitle = Titre;
	ofn.lpstrDefExt = Ext;
	ofn.Flags = 0;

	if (GetSaveFileName(&ofn)) return 1;

	return 0;
}


int Change_File_L(char *Dest, char *Dir, char *Titre, char *Filter, char *Ext)
{
	OPENFILENAME ofn;

	SetCurrentDirectory(Gens_Path);

	if (!strcmp(Dest, ""))
	{
		strcpy(Dest, "default.");
		strcat(Dest, Ext);
	}

	memset(&ofn, 0, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = HWnd;
	ofn.hInstance = ghInstance;
	ofn.lpstrFile = Dest;
	ofn.nMaxFile = 2047;
	ofn.lpstrFilter = Filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrInitialDir = Dir;
	ofn.lpstrTitle = Titre;
	ofn.lpstrDefExt = Ext;
	ofn.Flags = 0;

	if (GetOpenFileName(&ofn)) return 1;

	return 0;
}


int Change_Dir(char *Dest, char *Dir, char *Titre, char *Filter, char *Ext)
{
	OPENFILENAME ofn;
	int i;

	SetCurrentDirectory(Gens_Path);

	strcpy(Dest, "default.");
	strcat(Dest, Ext);
	memset(&ofn, 0, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = HWnd;
	ofn.hInstance = ghInstance;
	ofn.lpstrFile = Dest;
	ofn.nMaxFile = 2047;
	ofn.lpstrFilter = Filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrInitialDir = Dir;
	ofn.lpstrTitle = Titre;
	ofn.lpstrDefExt = Ext;
	ofn.Flags = 0;

	if (GetSaveFileName(&ofn))
	{
		i = strlen(Dest) - 1;
		while ((i > 0) && (Dest[i] != '\\')) i--;
		if (!i) return 0;
		Dest[++i] = 0;
		return 1;
	}

	return 0;
}


void Get_State_File_Name(char *name)
{
	char Ext[5] = ".gsX";

	SetCurrentDirectory(Gens_Path);

	Ext[3] = '0' + Current_State;
	strcpy(name, State_Dir);
	strcat(name, Rom_Name);
	if (UseMovieStates && MainMovie.Status)
	{
		strcat(name," - ");
		char* slash = 
			((strrchr(MainMovie.FileName, '/') > strrchr(MainMovie.FileName, '\\')) ? 
			strrchr(MainMovie.FileName, '/') : 
			strrchr(MainMovie.FileName, '\\'));
		slash++;
		strcat(name,slash);
		name[strlen(name)-4] = '\0';
	}
	strcat(name, Ext);
}
FILE *Get_State_File()
{
	char Name[2048];

	SetCurrentDirectory(Gens_Path);

	Get_State_File_Name(Name);

	return fopen(Name, "rb");
}


int Load_State_From_Buffer(unsigned char *buf)
{
	if (!((Genesis_Started)||(SegaCD_Started)||(_32X_Started))) return 0;

		if ((MainMovie.Status == MOVIE_PLAYING) || (MainMovie.Status == MOVIE_FINISHED))
		{
			fseek(MainMovie.File,0,SEEK_END);
			MainMovie.LastFrame = ((ftell(MainMovie.File) - 64)/3);
		}
		z80_Reset(&M_Z80); // note: this alters some of the game's state that is not restored by the following. Doesn't appear to cause desync, but...
////		main68k_reset();
////		YM2612ResetChip(0);
		Reset_VDP(); // Modif N - enabled, because the game locks up sometimes on loading a point with different sound samples (as often as about 1 out of every 20 tries in some games) in, for example, Ecco

		buf += Import_Genesis(buf); //upthmodif - fixed for new, additive, length determination
		if (SegaCD_Started)
		{
			Import_SegaCD(buf); // Uncommented - function now exists
			buf += SEGACD_LENGTH_EX; //upthmodif - fixed for new, additive, length determination
			z80_Reset(&M_Z80); //Unbarfs the Z80
		}
		if (_32X_Started)
		{
			Import_32X(buf);
			buf += G32X_LENGTH_EX; //upthmodif - fixed for new, additive, length determination
		}

//		Flag_Clr_Scr = 1;
//		CRam_Flag = 1;
//		VRam_Flag = 1;
	return (int) buf;
}
int Load_State(char *Name)
{
	FILE *f;
	unsigned char *buf;
	int len;
	unsigned short *palbuf;

	len = GENESIS_STATE_LENGTH + GENESIS_LENGTH_EX;
	if (Genesis_Started); //So it doesn't return zero if the SegaCD and 32X aren't running
	else if (SegaCD_Started) len += SEGACD_LENGTH_EX;
	else if (_32X_Started) len += G32X_LENGTH_EX;
	else return 0;
	if(MainMovie.Status==MOVIE_RECORDING)
	{
		MainMovie.NbRerecords++;
		if (MainMovie.TriplePlayerHack)
			FrameCount=max(max(max(Track1_FrameCount,Track2_FrameCount),Track3_FrameCount),FrameCount);
		else
			FrameCount=max(max(Track1_FrameCount,Track2_FrameCount),FrameCount);
		MainMovie.LastFrame=FrameCount;
		fseek(MainMovie.File,0,SEEK_SET);
		char *tempbuf = new char[64 + (FrameCount * 3)];
		fread(tempbuf,1,64 + (FrameCount * 3),MainMovie.File);
		fclose(MainMovie.File);
		MainMovie.File = fopen(MainMovie.FileName,"wb");
		fseek(MainMovie.File,0,SEEK_SET);
		fwrite(tempbuf,1,64 + (FrameCount * 3),MainMovie.File);
		WriteMovieHeader(&MainMovie);
		fclose(MainMovie.File);
		MainMovie.File = fopen(MainMovie.FileName,"r+b");
		delete[] tempbuf;
	}

	buf = State_Buffer;

	if ((f = fopen(Name, "rb")) == NULL) return 0;

	memset(buf, 0, len);
	if (fread(buf, 1, len, f))
	{
		if ((len = Load_State_From_Buffer(buf)) == 0)
			return 0;
//		fread(&x,4,1,f);
//		fread(&xg,4,1,f);
//		fread(&y,4,1,f);
//		fread(&yg,4,1,f);
		int switched = 0; //Modif N - switched is for displaying "switched to playback" message
		if ((MainMovie.File) && !(FrameCount < MainMovie.LastFrame))
		{
			MainMovie.Status = MOVIE_FINISHED;
			switched = 2;
		}
		if((MainMovie.File) && (MainMovie.Status != MOVIE_PLAYING)
			&& (MainMovie.ReadOnly == 1) && (FrameCount < MainMovie.LastFrame)) //Modif Upth - Allow switching to playback even if movie finished, if it hasn't been closed //Modif N - allow switching to playback if loading a state while recording in read-only mode
		{
			if (MainMovie.Status == MOVIE_RECORDING)
				WriteMovieHeader(&MainMovie);
			MainMovie.Status=MOVIE_PLAYING;
			switched = 1;
		}
	    if((MainMovie.File) && (MainMovie.Status != MOVIE_RECORDING) 
			&& (MainMovie.ReadOnly == 0)) //upthmodif - allow resume record even if we accidentally played past the end; so long as AutoCloseMovie is unchecked.
		{
			if(AutoBackupEnabled)
			{
				strncpy(Str_Tmp,MainMovie.FileName,512);
				strcat(MainMovie.FileName,".gmv");
				MainMovie.FileName[strlen(MainMovie.FileName)-7]='b';
				MainMovie.FileName[strlen(MainMovie.FileName)-6]='a';
				MainMovie.FileName[strlen(MainMovie.FileName)-5]='k';
				BackupMovieFile(&MainMovie);
				strncpy(MainMovie.FileName,Str_Tmp,512);
			}
			MainMovie.Status=MOVIE_RECORDING;
			sprintf(Str_Tmp, "STATE %d LOADED : RECORDING RESUMED", Current_State);
		}
		else
		{
			if (!(switched)) 
				sprintf(Str_Tmp, "STATE %d LOADED", Current_State);
			else if(switched == 1) //Modif N - "switched to playback" message
				sprintf(Str_Tmp, "STATE %d LOADED : SWITCHED TO PLAYBACK", Current_State);
			else if(switched == 2) //Modif N - "switched to playback" message
				sprintf(Str_Tmp, "STATE %d LOADED : MOVIE FINISHED", Current_State);
		}
		Put_Info(Str_Tmp, 2000);
	}

	//Modif N - bulletproof re-recording (loading)
	if((MainMovie.Status == MOVIE_RECORDING) && MainMovie.File)
	{
		unsigned long int temp = MainMovie.LastFrame;
		if (MainMovie.TriplePlayerHack)
		{
			if (track & TRACK1) Track1_FrameCount = max(temp,Track1_FrameCount);
			if (track & TRACK2) Track2_FrameCount = max(temp,Track2_FrameCount);
			if (track & TRACK3) Track3_FrameCount = max(temp,Track3_FrameCount);
		}
		else
		{
			if (track & TRACK1) Track1_FrameCount = max(temp,Track1_FrameCount);
			if (track & TRACK2) Track2_FrameCount = max(temp,Track2_FrameCount);
		}

		int m = fgetc(f);
		if(m == 'M' && !feof(f) && !ferror(f))
		{
			int pos = ftell(MainMovie.File);
			fseek(MainMovie.File,64,SEEK_SET);

			char* bla = new char [FrameCount*3];
			if(FrameCount*3 == fread(bla, 1, FrameCount*3, f))
				fwrite(bla, 1, FrameCount*3, MainMovie.File);
			delete[] bla;

			fseek(MainMovie.File,pos,SEEK_SET);
		}
	}

	//Modif N - consistency checking (loading)
	if((MainMovie.Status == MOVIE_PLAYING) && MainMovie.File)
	{
		bool inconsistent = false;
		unsigned long int temp = MainMovie.LastFrame;
		Track1_FrameCount = Track2_FrameCount = temp;
		if (MainMovie.TriplePlayerHack)
			Track3_FrameCount = temp;

		int m = fgetc(f);
		if(m == 'M' && !feof(f) && !ferror(f))
		{
			int pos = ftell(MainMovie.File);
			fseek(MainMovie.File,64,SEEK_SET);

			char* bla = new char [FrameCount*3]; // savestate movie input data
			char* bla2 = new char [FrameCount*3]; // playing movie input data
			if((FrameCount*3 != fread(bla, 1, FrameCount*3, f)) 
			|| (FrameCount*3 != fread(bla2, 1, FrameCount*3, MainMovie.File)) 
 			|| memcmp(bla,bla2,FrameCount*3))
				inconsistent = true;
			delete[] bla;
			delete[] bla2;

			fseek(MainMovie.File,pos,SEEK_SET);
		}
		if(inconsistent)
		{
			if (MessageBox(NULL, "The state you have loaded is inconsistent with this movie.\n Resume play anyway?", "Desync Warning", MB_YESNO | MB_ICONWARNING) != IDYES) 
			{
				MainMovie.Status = MOVIE_FINISHED;
				sprintf(Str_Tmp, "STATE %d LOADED : MOVIE FINISHED", Current_State);
				Put_Info(Str_Tmp, 2000);
			}
		}
	}
	fclose(f);
	if ((MainMovie.Status == MOVIE_PLAYING) || (MainMovie.Status == MOVIE_FINISHED))
	{
		fseek(MainMovie.File,-64,SEEK_END);
		MainMovie.LastFrame = (ftell(MainMovie.File) / 3);
	}
	Update_RAM_Search();

	return Show_Genesis_Screen(HWnd);
}
int Save_State_To_Buffer (unsigned char *buf)
{
	int len;

	len = GENESIS_STATE_LENGTH + GENESIS_LENGTH_EX; //Upthmodif - tweaked the length determination system;Modif N - used to be GENESIS_STATE_FILE_LENGTH, which I think is a major bug because then the amount written and the amount read are different - this change was necessary to append anything to the save (i.e. for bulletproof re-recording)
	if (SegaCD_Started) len += SEGACD_LENGTH_EX; //upthmodif - fixed for new, additive, length determination
	else if (_32X_Started) len += G32X_LENGTH_EX; //upthmodif - fixed for new, additive, length determination

	memset(buf, 0, len);
	Export_Genesis(buf);
	buf += GENESIS_STATE_LENGTH + GENESIS_LENGTH_EX; //upthmodif - fixed for new, additive, length determination
	if (SegaCD_Started)
	{
		Export_SegaCD(buf); //upthmodif - uncommented, function now exiss
		buf += SEGACD_LENGTH_EX; //upthmodif - fixed for new, additive, length determination
	}
	if (_32X_Started)
	{
		Export_32X(buf);
		buf += G32X_LENGTH_EX; //upthmodif - fixed for new, additive, length determination
	}

	return len;
}
int Save_State (char *Name)
{
	FILE *f;
	unsigned char *buf;
	int len;

	buf = State_Buffer;

	if ((f = fopen(Name, "wb")) == NULL) return 0;
	if  ((len = Save_State_To_Buffer(buf)) == 0) return 0;
	fwrite(State_Buffer, 1, len, f);
//	fwrite(&x,4,1,f);
//	fwrite(&xg,4,1,f);
//	fwrite(&y,4,1,f);
//	fwrite(&yg,4,1,f);

	//Modif N - bulletproof re-recording (saving)
	if(MainMovie.File && (MainMovie.Status != MOVIE_FINISHED) )
	{
		fputc('M', f);
		int pos = ftell(MainMovie.File);
		fseek(MainMovie.File,64,SEEK_SET);

		char* bla = new char [FrameCount*3];
		if(FrameCount*3 == fread(bla, 1, FrameCount*3, MainMovie.File))
			fwrite(bla, 1, FrameCount*3, f);
		delete[] bla;

		fseek(MainMovie.File,pos,SEEK_SET);
	}
	else
	{
		fputc('\0', f);
	}

	fclose(f);

	sprintf(Str_Tmp, "STATE %d SAVED", Current_State);
	Put_Info(Str_Tmp, 2000);

	return 1;
}

//Modif N. - added ImportData and ExportData because the import/export code was getting a little hairy without these
// The main advantage to using these, besides less lines of code, is that
// you can replace ImportData with ExportData, without changing anything else in the arguments,
// to go from import code to export code.
inline void ImportData(void* into, const void* data, unsigned int offset, unsigned int numBytes)
{
	unsigned char* dst = (unsigned char *) into;
	const unsigned char* src = ((const unsigned char*) data) + offset;
	while(numBytes--) *dst++ = *src++;
}
inline void ExportData(const void* from, void* data, unsigned int offset, unsigned int numBytes)
{
	const unsigned char* src = (const unsigned char *) from;
	unsigned char* dst = ((unsigned char*) data) + offset;
	while(numBytes--) *dst++ = *src++;
}
// versions that auto-increment the offset
inline void ImportDataAuto(void* into, const void* data, unsigned int & offset, unsigned int numBytes)
{
	ImportData(into, data, offset, numBytes);
	offset += numBytes;
}
inline void ExportDataAuto(const void* from, void* data, unsigned int & offset, unsigned int numBytes)
{
	ExportData(from, data, offset, numBytes);
	offset += numBytes;
}

/*

GST genecyst save file

Range        Size   Description
-----------  -----  -----------
00000-00002  3      "GST"
00006-00007  2      "\xE0\x40"
000FA-00112  24     VDP registers
00112-00191  128    Color RAM
00192-001E1  80     Vertical scroll RAM
001E4-003E3  512    YM2612 registers
00474-02473  8192   Z80 RAM
02478-12477  65536  68K RAM
12478-22477  65536  Video RAM

main 68000 registers
--------------------

00080-0009F : D0-D7
000A0-000BF : A0-A7
000C8 : PC
000D0 : SR
000D2 : USP
000D6 : SSP

Z80 registers
-------------

00404 : AF
00408 : BC
0040C : DE
00410 : HL
00414 : IX
00418 : IY
0041C : PC
00420 : SP
00424 : AF'
00428 : BC'
0042C : DE'
00430 : HL'

00434 : I
00435 : Unknow
00436 : IFF1 = IFF2
00437 : Unknow

The 'R' register is not supported.

Z80 State
---------

00438 : Z80 RESET
00439 : Z80 BUSREQ
0043A : Unknow
0043B : Unknow

0043C : Z80 BANK (DWORD)

Gens and Kega ADD
-----------------

00040 : last VDP Control data written (DWORD)
00044 : second write flag (1 for second write) 
00045 : DMA Fill flag (1 mean next data write will cause a DMA fill)
00048 : VDP write address (DWORD)

00050 : Version       (Genecyst=0 ; Kega=5 ; Gens=5)
00051 : Emulator ID   (Genecyst=0 ; Kega=0 ; Gens=1)
00052 : System ID     (Genesis=0 ; SegaCD=1 ; 32X=2 ; SegaCD32X=3)

00060-00070 : PSG registers (WORD).


SEGA CD
-------

+00000-00FFF : Gate array & sub 68K
+01000-80FFF : Prg RAM
+81000-C0FFF : Word RAM (2M mode arrangement)
+C1000-D0FFF : PCM RAM
+D1000-DFFFF : CDD & CDC data (16 kB cache include)

32X
---

main SH2
--------

+00000-00FFF : cache
+01000-011FF : IO registers
+01200-0123F : R0-R15
+01240 : SR
+01244 : GBR
+01248 : VBR
+0124C : MACL
+01250 : MACH
+01254 : PR
+01258 : PC
+0125C : State

sub SH2
-------

+01400-023FF : cache
+02400-025FF : IO registers
+02600-0263F : R0-R15
+02640 : SR
+02644 : GBR
+02648 : VBR
+0264C : MACL
+02650 : MACH
+02654 : PR
+02658 : PC
+0265C : State

others
------
// Fix 32X save state :
// enregistrer correctement les registres syst�mes ...

+02700 : ADEN bit (bit 0)
+02701 : FM bit (bit 7)
+02702 : Master SH2 INT mask register
+02703 : Slave SH2 INT mask register
+02704 : 68000 32X rom bank register
+02705 : RV (Rom to VRAM DMA allowed) bit (bit 0)
+02710-0273F : FIFO stuff (not yet done)
+02740-0274F : 32X communication buffer
+02750-02759 : PWM registers
+02760-0276F : 32X VDP registers
+02800-029FF : 32X palette
+02A00-429FF : SDRAM
+42A00-829FF : FB1 & FB2

*/

int Import_Genesis(unsigned char *Data)
{
	unsigned char Reg_1[0x200], Version, *src;
	int i;

//	VDP_Int = 0;
//	DMAT_Length = 0;
	int len = GENESIS_STATE_LENGTH + GENESIS_LENGTH_EX;
	Version = Data[0x50];
	if (Version < 6) len -= 0x10000;

	ImportData(CRam, Data, 0x112, 0x80);
	ImportData(VSRam, Data, 0x192, 0x50);
	ImportData(Ram_Z80, Data, 0x474, 0x2000);
	
	for(i = 0; i < 0x10000; i += 2)
	{
		Ram_68k[i + 0] = Data[i + 0x2478 + 1];
		Ram_68k[i + 1] = Data[i + 0x2478 + 0];
	}

	for(i = 0; i < 0x10000; i += 2)
	{
		VRam[i + 0] = Data[i + 0x12478 + 1];
		VRam[i + 1] = Data[i + 0x12478 + 0];
	}

	ImportData(Reg_1, Data, 0x1E4, 0x200);
	YM2612_Restore(Reg_1);

	if ((Version >= 2) && (Version < 4))
	{
		ImportData(&Ctrl, Data, 0x30, 7 * 4);

		Z80_State &= ~6;
		if (Data[0x440] & 1) Z80_State |= 2;
		if (Data[0x444] & 1) Z80_State |= 4;

		ImportData(&Bank_Z80, Data, 0x448, 4);

		ImportData(&PSG_Save, Data, 0x224B8, 8 * 4);
		PSG_Restore_State();
	}
	else if ((Version >= 4) || (Version == 0)) 		// New version compatible with Kega.
	{
		Z80_State &= ~6;

		if (Version == 4)
		{
			M_Z80.IM = Data[0x437];
			M_Z80.IFF.b.IFF1 = (Data[0x438] & 1) << 2;
			M_Z80.IFF.b.IFF2 = (Data[0x438] & 1) << 2;

			Z80_State |= (Data[0x439] & 1) << 1;
		}
		else
		{
			M_Z80.IM = 1;
			M_Z80.IFF.b.IFF1 = (Data[0x436] & 1) << 2;
			M_Z80.IFF.b.IFF2 = (Data[0x436] & 1) << 2;

			Z80_State |= ((Data[0x439] & 1) ^ 1) << 1;
			Z80_State |= ((Data[0x438] & 1) ^ 1) << 2;
		}

		src = (unsigned char *) &Ctrl;
		for(i = 0; i < 7 * 4; i++) *src++ = 0;

		Write_VDP_Ctrl(Data[0x40] + (Data[0x41] << 8));
		Write_VDP_Ctrl(Data[0x42] + (Data[0x43] << 8));

		Ctrl.Flag = Data[0x44];
		Ctrl.DMA = (Data[0x45] & 1) << 2;
		Ctrl.Access = Data[0x46] + (Data[0x47] << 8); //Nitsuja added this
		Ctrl.Address = Data[0x48] + (Data[0x49] << 8);
		
		ImportData(&Bank_Z80, Data, 0x43C, 4);

		if (Version >= 4)
		{
			for(i = 0; i < 8; i++) PSG_Save[i] = Data[i * 2 + 0x60] + (Data[i * 2 + 0x61] << 8);
			PSG_Restore_State();
		}
	}

	z80_Set_AF(&M_Z80, Data[0x404] + (Data[0x405] << 8));
	M_Z80.AF.b.FXY = Data[0x406]; //Modif N
	M_Z80.BC.w.BC = Data[0x408] + (Data[0x409] << 8);
	M_Z80.DE.w.DE = Data[0x40C] + (Data[0x40D] << 8);
	M_Z80.HL.w.HL = Data[0x410] + (Data[0x411] << 8);
	M_Z80.IX.w.IX = Data[0x414] + (Data[0x415] << 8);
	M_Z80.IY.w.IY = Data[0x418] + (Data[0x419] << 8);
	z80_Set_PC(&M_Z80, Data[0x41C] + (Data[0x41D] << 8));
	M_Z80.SP.w.SP = Data[0x420] + (Data[0x421] << 8);
	z80_Set_AF2(&M_Z80, Data[0x424] + (Data[0x425] << 8));
	M_Z80.BC2.w.BC2 = Data[0x428] + (Data[0x429] << 8);
	M_Z80.DE2.w.DE2 = Data[0x42C] + (Data[0x42D] << 8);
	M_Z80.HL2.w.HL2 = Data[0x430] + (Data[0x431] << 8);
	M_Z80.I = Data[0x434] & 0xFF;

	FrameCount=Data[0x22478]+(Data[0x22479]<<8)+(Data[0x2247A]<<16)+(Data[0x2247B]<<24);

	main68k_GetContext(&Context_68K);

	for(i = 0; i < 24; i++) Set_VDP_Reg(i, Data[0xFA + i]);
	
	ImportData(&Context_68K.dreg[0], Data, 0x80, 8 * 2 * 4);

	ImportData(&Context_68K.pc, Data, 0xC8, 4);

	ImportData(&Context_68K.sr, Data, 0xD0, 2);

	if ((Version >= 3) || (Version == 0))
	{
		if (Data[0xD1] & 0x20)
		{
			// Supervisor
			ImportData(&Context_68K.asp, Data, 0xD2, 2);
		}
		else
		{
			// User
			ImportData(&Context_68K.asp, Data, 0xD6, 2);
		}
	}

	if(Version >= 6)
	{
		//Modif N. - saving more stuff (although a couple of these are saved above in a weird way that I don't trust)
		unsigned int offset = 0x2247C;

		ImportDataAuto(&Context_68K.dreg, Data, offset, 4*8);
		ImportDataAuto(&Context_68K.areg, Data, offset, 4*8);
		ImportDataAuto(&Context_68K.asp, Data, offset, 4);
		ImportDataAuto(&Context_68K.pc, Data, offset, 4);
		ImportDataAuto(&Context_68K.odometer, Data, offset, 4);
		ImportDataAuto(&Context_68K.interrupts, Data, offset, 8);
		ImportDataAuto(&Context_68K.sr, Data, offset, 2);
		ImportDataAuto(&Context_68K.contextfiller00, Data, offset, 2);

		ImportDataAuto(&VDP_Reg.H_Int, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.Set1, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.Set2, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.Pat_ScrA_Adr, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.Pat_ScrA_Adr, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.Pat_Win_Adr, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.Pat_ScrB_Adr, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.Spr_Att_Adr, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.Reg6, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.BG_Color, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.Reg8, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.Reg9, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.H_Int, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.Set3, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.Set4, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.H_Scr_Adr, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.Reg14, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.Auto_Inc, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.Scr_Size, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.Win_H_Pos, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.Win_V_Pos, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.DMA_Length_L, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.DMA_Length_H, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.DMA_Src_Adr_L, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.DMA_Src_Adr_M, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.DMA_Src_Adr_H, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.DMA_Length, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.DMA_Address, Data, offset, 4);

		ImportDataAuto(&Controller_1_Counter, Data, offset, 4);
		ImportDataAuto(&Controller_1_Delay, Data, offset, 4);
		ImportDataAuto(&Controller_1_State, Data, offset, 4);
		ImportDataAuto(&Controller_1_COM, Data, offset, 4);
		ImportDataAuto(&Controller_2_Counter, Data, offset, 4);
		ImportDataAuto(&Controller_2_Delay, Data, offset, 4);
		ImportDataAuto(&Controller_2_State, Data, offset, 4);
		ImportDataAuto(&Controller_2_COM, Data, offset, 4);
		ImportDataAuto(&Memory_Control_Status, Data, offset, 4);
		ImportDataAuto(&Cell_Conv_Tab, Data, offset, 4);
		ImportDataAuto(&Controller_1_Type, Data, offset, 4);
		ImportDataAuto(&Controller_1_Up, Data, offset, 4);
		ImportDataAuto(&Controller_1_Down, Data, offset, 4);
		ImportDataAuto(&Controller_1_Left, Data, offset, 4);
		ImportDataAuto(&Controller_1_Right, Data, offset, 4);
		ImportDataAuto(&Controller_1_Start, Data, offset, 4);
		ImportDataAuto(&Controller_1_Mode, Data, offset, 4);
		ImportDataAuto(&Controller_1_A, Data, offset, 4);
		ImportDataAuto(&Controller_1_B, Data, offset, 4);
		ImportDataAuto(&Controller_1_C, Data, offset, 4);
		ImportDataAuto(&Controller_1_X, Data, offset, 4);
		ImportDataAuto(&Controller_1_Y, Data, offset, 4);
		ImportDataAuto(&Controller_1_Z, Data, offset, 4);
		ImportDataAuto(&Controller_2_Type, Data, offset, 4);
		ImportDataAuto(&Controller_2_Up, Data, offset, 4);
		ImportDataAuto(&Controller_2_Down, Data, offset, 4);
		ImportDataAuto(&Controller_2_Left, Data, offset, 4);
		ImportDataAuto(&Controller_2_Right, Data, offset, 4);
		ImportDataAuto(&Controller_2_Start, Data, offset, 4);
		ImportDataAuto(&Controller_2_Mode, Data, offset, 4);
		ImportDataAuto(&Controller_2_A, Data, offset, 4);
		ImportDataAuto(&Controller_2_B, Data, offset, 4);
		ImportDataAuto(&Controller_2_C, Data, offset, 4);
		ImportDataAuto(&Controller_2_X, Data, offset, 4);
		ImportDataAuto(&Controller_2_Y, Data, offset, 4);
		ImportDataAuto(&Controller_2_Z, Data, offset, 4);

		ImportDataAuto(&DMAT_Length, Data, offset, 4);
		ImportDataAuto(&DMAT_Type, Data, offset, 4);
		ImportDataAuto(&DMAT_Tmp, Data, offset, 4);
		ImportDataAuto(&VDP_Current_Line, Data, offset, 4);
		ImportDataAuto(&VDP_Num_Vis_Lines, Data, offset, 4);
		ImportDataAuto(&VDP_Num_Vis_Lines, Data, offset, 4);
		ImportDataAuto(&Bank_M68K, Data, offset, 4);
		ImportDataAuto(&S68K_State, Data, offset, 4);
		ImportDataAuto(&Z80_State, Data, offset, 4);
		ImportDataAuto(&Last_BUS_REQ_Cnt, Data, offset, 4);
		ImportDataAuto(&Last_BUS_REQ_St, Data, offset, 4);
		ImportDataAuto(&Fake_Fetch, Data, offset, 4);
		ImportDataAuto(&Game_Mode, Data, offset, 4);
		ImportDataAuto(&CPU_Mode, Data, offset, 4);
		ImportDataAuto(&CPL_M68K, Data, offset, 4);
		ImportDataAuto(&CPL_S68K, Data, offset, 4);
		ImportDataAuto(&CPL_Z80, Data, offset, 4);
		ImportDataAuto(&Cycles_S68K, Data, offset, 4);
		ImportDataAuto(&Cycles_M68K, Data, offset, 4);
		ImportDataAuto(&Cycles_Z80, Data, offset, 4);
	ImportDataAuto(&VDP_Status, Data, offset, 4);
	ImportDataAuto(&VDP_Int, Data, offset, 4);
		ImportDataAuto(&Ctrl.Write, Data, offset, 4);
		ImportDataAuto(&Ctrl.DMA_Mode, Data, offset, 4);
		ImportDataAuto(&Ctrl.DMA, Data, offset, 4);
		//ImportDataAuto(&CRam_Flag, Data, offset, 4); //Causes screen to blank
		//offset+=4;
		ImportDataAuto(&LagCount, Data, offset, 4);
		ImportDataAuto(&VRam_Flag, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.DMA_Length, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.Auto_Inc, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.DMA_Length, Data, offset, 4);
	////	ImportDataAuto(VRam, Data, offset, 65536);
		ImportDataAuto(CRam, Data, offset, 512);
	////	ImportDataAuto(VSRam, Data, offset, 64);
		ImportDataAuto(H_Counter_Table, Data, offset, 512 * 2);
	////	ImportDataAuto(Spr_Link, Data, offset, 4*256);
	////	extern int DMAT_Tmp, VSRam_Over;
	////	ImportDataAuto(&DMAT_Tmp, Data, offset, 4);
	////	ImportDataAuto(&VSRam_Over, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.DMA_Length_L, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.DMA_Length_H, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.DMA_Src_Adr_L, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.DMA_Src_Adr_M, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.DMA_Src_Adr_H, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.DMA_Length, Data, offset, 4);
		ImportDataAuto(&VDP_Reg.DMA_Address, Data, offset, 4);
	}

	main68k_SetContext(&Context_68K);
	return len;
}

void Export_Genesis(unsigned char *Data)
{
	unsigned char Reg_1[0x200], *src;
	int i;

#ifdef _WIN32
	// warnings disabled because they were highly annoying and not once have I encountered a desync from ignoring them
	//if(DMAT_Length)
	//{
	//	MessageBox(NULL, "Saving during DMA transfer; savestate may be corrupt. Try advancing the frame and saving again.", "Warning", MB_OK | MB_ICONWARNING);
	//}
	//if(VDP_Int)
	//{
	//	MessageBox(NULL, "Saving during VDP interrupt; savestate may be corrupt. Try advancing the frame and saving again.", "Warning", MB_OK | MB_ICONWARNING);
	//}
#endif

	// XXX: should save the state of DMA transfer, instead... actually, maybe it is saved already now
//	while (DMAT_Length) Update_DMA();		// Be sure to finish DMA before save

	Data[0x00] = 'G';
	Data[0x01] = 'S';
	Data[0x02] = 'T';
	Data[0x03] = 0x40;
	Data[0x04] = 0xE0;

	Data[0x50] = 6;		// Version
	Data[0x51] = 0;		// Gens

	PSG_Save_State();

	for(i = 0; i < 8; i++)
	{
		Data[0x60 + i * 2] = PSG_Save[i] & 0xFF;
		Data[0x61 + i * 2] = (PSG_Save[i] >> 8) & 0xFF;
	}

	main68k_GetContext(&Context_68K);

	ExportData(&Context_68K.dreg[0], Data, 0x80, 8 * 2 * 4);

	ExportData(&Context_68K.pc, Data, 0xC8, 4);

	ExportData(&Context_68K.sr, Data, 0xD0, 2);

	if (Context_68K.sr & 0x2000)
	{
		ExportData(&Context_68K.asp, Data, 0xD2, 4);
		ExportData(&Context_68K.areg[7], Data, 0xD6, 4);
	}
	else
	{
		ExportData(&Context_68K.asp, Data, 0xD6, 4);
		ExportData(&Context_68K.areg[7], Data, 0xD2, 4);
	}

	ExportData(&Ctrl.Data, Data, 0x40, 4);

	Data[0x44] = Ctrl.Flag;
	Data[0x45] = (Ctrl.DMA >> 2) & 1;

	Data[0x46] = Ctrl.Access & 0xFF; //Nitsuja added this
	Data[0x47] = (Ctrl.Access >> 8) & 0xFF; //Nitsuja added this

	Data[0x48] = Ctrl.Address & 0xFF;
	Data[0x49] = (Ctrl.Address >> 8) & 0xFF;

	VDP_Reg.DMA_Length_L = VDP_Reg.DMA_Length & 0xFF;
	VDP_Reg.DMA_Length_H = (VDP_Reg.DMA_Length >> 8) & 0xFF;

	VDP_Reg.DMA_Src_Adr_L = VDP_Reg.DMA_Address & 0xFF;
	VDP_Reg.DMA_Src_Adr_M = (VDP_Reg.DMA_Address >> 8) & 0xFF;
	VDP_Reg.DMA_Src_Adr_H = (VDP_Reg.DMA_Address >> 16) & 0xFF;

	VDP_Reg.DMA_Src_Adr_H |= Ctrl.DMA_Mode & 0xC0;

	src = (unsigned char *) &(VDP_Reg.Set1);
	for(i = 0; i < 24; i++)
	{
		Data[0xFA + i] = *src;
		src += 4;
	}

	for(i = 0; i < 0x80; i++) Data[i + 0x112] = CRam[i] & 0xFF;
	for(i = 0; i < 0x50; i++) Data[i + 0x192] = VSRam[i]  & 0xFF;

	YM2612_Save(Reg_1);
	for(i = 0; i < 0x200; i++) Data[i + 0x1E4] = Reg_1[i];

	Data[0x404] = (unsigned char) (z80_Get_AF(&M_Z80) & 0xFF);
	Data[0x405] = (unsigned char) (z80_Get_AF(&M_Z80) >> 8);
	Data[0x406] = (unsigned char) (M_Z80.AF.b.FXY & 0xFF); //Modif N
	Data[0x407] = (unsigned char) 0; //Modif N
	Data[0x408] = (unsigned char) (M_Z80.BC.w.BC & 0xFF);
	Data[0x409] = (unsigned char) (M_Z80.BC.w.BC >> 8);
	Data[0x40C] = (unsigned char) (M_Z80.DE.w.DE & 0xFF);
	Data[0x40D] = (unsigned char) (M_Z80.DE.w.DE >> 8);
	Data[0x410] = (unsigned char) (M_Z80.HL.w.HL & 0xFF);
	Data[0x411] = (unsigned char) (M_Z80.HL.w.HL >> 8);
	Data[0x414] = (unsigned char) (M_Z80.IX.w.IX & 0xFF);
	Data[0x415] = (unsigned char) (M_Z80.IX.w.IX >> 8);
	Data[0x418] = (unsigned char) (M_Z80.IY.w.IY & 0xFF);
	Data[0x419] = (unsigned char) (M_Z80.IY.w.IY >> 8);
	Data[0x41C] = (unsigned char) (z80_Get_PC(&M_Z80) & 0xFF);
	Data[0x41D] = (unsigned char) (z80_Get_PC(&M_Z80) >> 8);
	Data[0x420] = (unsigned char) (M_Z80.SP.w.SP & 0xFF);
	Data[0x421] = (unsigned char) (M_Z80.SP.w.SP >> 8);
	Data[0x424] = (unsigned char) (z80_Get_AF2(&M_Z80) & 0xFF);
	Data[0x425] = (unsigned char) (z80_Get_AF2(&M_Z80) >> 8);
	Data[0x428] = (unsigned char) (M_Z80.BC2.w.BC2 & 0xFF);
	Data[0x429] = (unsigned char) (M_Z80.BC2.w.BC2 >> 8);
	Data[0x42C] = (unsigned char) (M_Z80.DE2.w.DE2 & 0xFF);
	Data[0x42D] = (unsigned char) (M_Z80.DE2.w.DE2 >> 8);
	Data[0x430] = (unsigned char) (M_Z80.HL2.w.HL2 & 0xFF);
	Data[0x431] = (unsigned char) (M_Z80.HL2.w.HL2 >> 8);
	Data[0x434] = (unsigned char) (M_Z80.I);
	Data[0x436] = (unsigned char) (M_Z80.IFF.b.IFF1 >> 2);

	Data[0x438] = (unsigned char) (((Z80_State & 4) >> 2) ^ 1);
	Data[0x439] = (unsigned char) (((Z80_State & 2) >> 1) ^ 1);

	ExportData(&Bank_Z80, Data, 0x43C, 4);

	for(i = 0; i < 0x2000; i++) Data[i + 0x474] = Ram_Z80[i];

	for(i = 0; i < 0x10000; i += 2)
	{
		Data[i + 0x2478 + 1] = Ram_68k[i + 0];
		Data[i + 0x2478 + 0] = Ram_68k[i + 1];
	}

	for(i = 0; i < 0x10000; i += 2)
	{
		Data[i + 0x12478 + 1] = VRam[i + 0];
		Data[i + 0x12478 + 0] = VRam[i + 1];
	}
	Data[0x22478]=unsigned char (FrameCount&0xFF);	//Modif
	Data[0x22479]=unsigned char ((FrameCount>>8)&0xFF);	//Modif
	Data[0x2247A]=unsigned char ((FrameCount>>16)&0xFF);	//Modif
	Data[0x2247B]=unsigned char ((FrameCount>>24)&0xFF);	//Modif

//	if(Version >= 6)
	{
		//Modif N. - saving more stuff (although a couple of these are saved above in a weird way that I don't trust)
		unsigned int offset = 0x2247C;

		ExportDataAuto(&Context_68K.dreg, Data, offset, 4*8);
		ExportDataAuto(&Context_68K.areg, Data, offset, 4*8);
		ExportDataAuto(&Context_68K.asp, Data, offset, 4);
		ExportDataAuto(&Context_68K.pc, Data, offset, 4);
		ExportDataAuto(&Context_68K.odometer, Data, offset, 4);
		ExportDataAuto(&Context_68K.interrupts, Data, offset, 8);
		ExportDataAuto(&Context_68K.sr, Data, offset, 2);
		ExportDataAuto(&Context_68K.contextfiller00, Data, offset, 2);

		ExportDataAuto(&VDP_Reg.H_Int, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.Set1, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.Set2, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.Pat_ScrA_Adr, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.Pat_ScrA_Adr, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.Pat_Win_Adr, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.Pat_ScrB_Adr, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.Spr_Att_Adr, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.Reg6, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.BG_Color, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.Reg8, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.Reg9, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.H_Int, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.Set3, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.Set4, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.H_Scr_Adr, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.Reg14, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.Auto_Inc, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.Scr_Size, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.Win_H_Pos, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.Win_V_Pos, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.DMA_Length_L, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.DMA_Length_H, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.DMA_Src_Adr_L, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.DMA_Src_Adr_M, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.DMA_Src_Adr_H, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.DMA_Length, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.DMA_Address, Data, offset, 4);

		ExportDataAuto(&Controller_1_Counter, Data, offset, 4);
		ExportDataAuto(&Controller_1_Delay, Data, offset, 4);
		ExportDataAuto(&Controller_1_State, Data, offset, 4);
		ExportDataAuto(&Controller_1_COM, Data, offset, 4);
		ExportDataAuto(&Controller_2_Counter, Data, offset, 4);
		ExportDataAuto(&Controller_2_Delay, Data, offset, 4);
		ExportDataAuto(&Controller_2_State, Data, offset, 4);
		ExportDataAuto(&Controller_2_COM, Data, offset, 4);
		ExportDataAuto(&Memory_Control_Status, Data, offset, 4);
		ExportDataAuto(&Cell_Conv_Tab, Data, offset, 4);
		ExportDataAuto(&Controller_1_Type, Data, offset, 4);
		ExportDataAuto(&Controller_1_Up, Data, offset, 4);
		ExportDataAuto(&Controller_1_Down, Data, offset, 4);
		ExportDataAuto(&Controller_1_Left, Data, offset, 4);
		ExportDataAuto(&Controller_1_Right, Data, offset, 4);
		ExportDataAuto(&Controller_1_Start, Data, offset, 4);
		ExportDataAuto(&Controller_1_Mode, Data, offset, 4);
		ExportDataAuto(&Controller_1_A, Data, offset, 4);
		ExportDataAuto(&Controller_1_B, Data, offset, 4);
		ExportDataAuto(&Controller_1_C, Data, offset, 4);
		ExportDataAuto(&Controller_1_X, Data, offset, 4);
		ExportDataAuto(&Controller_1_Y, Data, offset, 4);
		ExportDataAuto(&Controller_1_Z, Data, offset, 4);
		ExportDataAuto(&Controller_2_Type, Data, offset, 4);
		ExportDataAuto(&Controller_2_Up, Data, offset, 4);
		ExportDataAuto(&Controller_2_Down, Data, offset, 4);
		ExportDataAuto(&Controller_2_Left, Data, offset, 4);
		ExportDataAuto(&Controller_2_Right, Data, offset, 4);
		ExportDataAuto(&Controller_2_Start, Data, offset, 4);
		ExportDataAuto(&Controller_2_Mode, Data, offset, 4);
		ExportDataAuto(&Controller_2_A, Data, offset, 4);
		ExportDataAuto(&Controller_2_B, Data, offset, 4);
		ExportDataAuto(&Controller_2_C, Data, offset, 4);
		ExportDataAuto(&Controller_2_X, Data, offset, 4);
		ExportDataAuto(&Controller_2_Y, Data, offset, 4);
		ExportDataAuto(&Controller_2_Z, Data, offset, 4);

		ExportDataAuto(&DMAT_Length, Data, offset, 4);
		ExportDataAuto(&DMAT_Type, Data, offset, 4);
		ExportDataAuto(&DMAT_Tmp, Data, offset, 4);
		ExportDataAuto(&VDP_Current_Line, Data, offset, 4);
		ExportDataAuto(&VDP_Num_Vis_Lines, Data, offset, 4);
		ExportDataAuto(&VDP_Num_Vis_Lines, Data, offset, 4);
		ExportDataAuto(&Bank_M68K, Data, offset, 4);
		ExportDataAuto(&S68K_State, Data, offset, 4);
		ExportDataAuto(&Z80_State, Data, offset, 4);
		ExportDataAuto(&Last_BUS_REQ_Cnt, Data, offset, 4);
		ExportDataAuto(&Last_BUS_REQ_St, Data, offset, 4);
	ExportDataAuto(&Fake_Fetch, Data, offset, 4);
		ExportDataAuto(&Game_Mode, Data, offset, 4);
		ExportDataAuto(&CPU_Mode, Data, offset, 4);
		ExportDataAuto(&CPL_M68K, Data, offset, 4);
		ExportDataAuto(&CPL_S68K, Data, offset, 4);
		ExportDataAuto(&CPL_Z80, Data, offset, 4);
		ExportDataAuto(&Cycles_S68K, Data, offset, 4);
		ExportDataAuto(&Cycles_M68K, Data, offset, 4);
		ExportDataAuto(&Cycles_Z80, Data, offset, 4);
	ExportDataAuto(&VDP_Status, Data, offset, 4);
	ExportDataAuto(&VDP_Int, Data, offset, 4);
		ExportDataAuto(&Ctrl.Write, Data, offset, 4);
		ExportDataAuto(&Ctrl.DMA_Mode, Data, offset, 4);
		ExportDataAuto(&Ctrl.DMA, Data, offset, 4);
		//ExportDataAuto(&CRam_Flag, Data, offset, 4);
		//offset+=4;
		ExportDataAuto(&LagCount, Data, offset, 4);
		ExportDataAuto(&VRam_Flag, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.DMA_Length, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.Auto_Inc, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.DMA_Length, Data, offset, 4);
	////	ExportDataAuto(VRam, Data, offset, 65536);
		ExportDataAuto(CRam, Data, offset, 512);
	////	ExportDataAuto(VSRam, Data, offset, 64);
		ExportDataAuto(H_Counter_Table, Data, offset, 512 * 2);
	////	ExportDataAuto(Spr_Link, Data, offset, 4*256);
	////	extern int DMAT_Tmp, VSRam_Over;
	////	ExportDataAuto(&DMAT_Tmp, Data, offset, 4);
	////	ExportDataAuto(&VSRam_Over, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.DMA_Length_L, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.DMA_Length_H, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.DMA_Src_Adr_L, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.DMA_Src_Adr_M, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.DMA_Src_Adr_H, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.DMA_Length, Data, offset, 4);
		ExportDataAuto(&VDP_Reg.DMA_Address, Data, offset, 4);

	}
}
void Import_SegaCD(unsigned char *Data) 
{
	S68000CONTEXT Context_sub68K;
	unsigned char *src;
	int i,j;

	sub68k_GetContext(&Context_sub68K);

	//sub68K bit goes here
		ImportData(&(Context_sub68K.dreg[0]), Data, 0x0, 8 * 4);
		ImportData(&(Context_sub68K.areg[0]), Data, 0x20, 8 * 4);
		ImportData(&(Context_sub68K.pc), Data, 0x48, 4);
		ImportData(&(Context_sub68K.sr), Data, 0x50, 2);
		
		if(Data[0x51] & 0x20)
		{
			// Supervisor
			ImportData(&Context_68K.asp, Data, 0x52, 4);
		}
		else
		{
			// User
			ImportData(&Context_68K.asp, Data, 0x56, 4);
		}

		
		ImportData(&(Context_sub68K.odometer), Data, 0x5A, 4);

		ImportData(Context_sub68K.interrupts, Data, 0x60, 8);

		ImportData(&Ram_Word_State, Data, 0x6C, 4);

	//here ends sub68k bit
	
	sub68k_SetContext(&Context_sub68K);
	
	//PCM Chip Load
		ImportData(&PCM_Chip.Rate, Data, 0x100, 4);
		ImportData(&PCM_Chip.Enable, Data, 0x104, 4);
		ImportData(&PCM_Chip.Cur_Chan, Data, 0x108, 4);
		ImportData(&PCM_Chip.Bank, Data, 0x10C, 4);

		for (j = 0; j < 8; j++)
		{
			ImportData(&PCM_Chip.Channel[j].ENV, Data, 0x120 + (j * 4 * 11), 4);
			ImportData(&PCM_Chip.Channel[j].PAN, Data, 0x124 + (j * 4 * 11), 4);
			ImportData(&PCM_Chip.Channel[j].MUL_L, Data, 0x128 + (j * 4 * 11), 4);
			ImportData(&PCM_Chip.Channel[j].MUL_R, Data, 0x12C + (j * 4 * 11), 4);
			ImportData(&PCM_Chip.Channel[j].St_Addr, Data, 0x130 + (j * 4 * 11), 4);
			ImportData(&PCM_Chip.Channel[j].Loop_Addr, Data, 0x134 + (j * 4 * 11), 4);
			ImportData(&PCM_Chip.Channel[j].Addr, Data, 0x138 + (j * 4 * 11), 4);
			ImportData(&PCM_Chip.Channel[j].Step, Data, 0x13C + (j * 4 * 11), 4);
			ImportData(&PCM_Chip.Channel[j].Step_B, Data, 0x140 + (j * 4 * 11), 4);
			ImportData(&PCM_Chip.Channel[j].Enable, Data, 0x144 + (j * 4 * 11), 4);
			ImportData(&PCM_Chip.Channel[j].Data, Data, 0x148 + (j * 4 * 11), 4);
		}
	//End PCM Chip Load

	//Init_RS_GFX(); //purge old GFX data
	//GFX State Load
		ImportData(&(Rot_Comp.Stamp_Size), Data, 0x300, 4);
		ImportData(&(Rot_Comp.Stamp_Map_Adr), Data, 0x304, 4);
		ImportData(&(Rot_Comp.IB_V_Cell_Size), Data, 0x308, 4);
		ImportData(&(Rot_Comp.IB_Adr), Data, 0x30C, 4);
		ImportData(&(Rot_Comp.IB_Offset), Data, 0x310, 4);
		ImportData(&(Rot_Comp.IB_H_Dot_Size), Data, 0x314, 4);
		ImportData(&(Rot_Comp.IB_V_Dot_Size), Data, 0x318, 4);
		ImportData(&(Rot_Comp.Vector_Adr), Data, 0x31C, 4);
		ImportData(&(Rot_Comp.Rotation_Running), Data, 0x320, 4);
	//End GFX State Load

	//gate array bit
		ImportData(&COMM.Flag, Data, 0x0500, 4);
		src = (unsigned char *) &COMM.Command;
		for (j = 0; j < 8 * 2; j+=2) for (i = 0; i < 2; i++) *src++ = Data[i + 0x0504 + j];
		src = (unsigned char *) &COMM.Status;
		for (j = 0; j < 8 * 2; j+=2) for (i = 0; i < 2; i++) *src++ = Data[i + 0x0514 + j];
		ImportData(&Memory_Control_Status, Data, 0x0524, 4);
		ImportData(&Init_Timer_INT3, Data, 0x0528, 4);
		ImportData(&Timer_INT3, Data, 0x052C, 4);
		ImportData(&Timer_Step, Data, 0x0530, 4);
		ImportData(&Int_Mask_S68K, Data, 0x0534, 4);
		ImportData(&Font_COLOR, Data, 0x0538, 4);
		ImportData(&Font_BITS, Data, 0x053C, 4);
		ImportData(&CD_Access_Timer, Data, 0x0540, 4);
		ImportData(&SCD.Status_CDC, Data, 0x0544, 4);
		ImportData(&SCD.Status_CDD, Data, 0x0548, 4);
		ImportData(&SCD.Cur_LBA, Data, 0x054C, 4);
		ImportData(&SCD.Cur_Track, Data, 0x0550, 4);
		ImportData(&S68K_Mem_WP, Data, 0x0554, 4);
		ImportData(&S68K_Mem_PM, Data, 0x0558, 4);
		// More goes here when found
	//here ends gate array bit

	//Misc Status Flags
		ImportData(&Ram_Word_State, Data, 0xF00, 4); //For determining 1M or 2M
		ImportData(&LED_Status, Data, 0xF08, 4); //So the LED shows up properly
	//Word RAM state

	ImportData(Ram_Prg, Data, 0x1000, 0x80000); //Prg RAM
	
	//Word RAM
		if (Ram_Word_State >= 2) ImportData(Ram_Word_1M, Data, 0x81000, 0x40000); //1M mode
		else ImportData(Ram_Word_2M, Data, 0x81000, 0x40000); //2M mode
		//ImportData(Ram_Word_2M, Data, 0x81000, 0x40000); //2M mode
	//Word RAM end

	ImportData(Ram_PCM, Data, 0xC1000, 0x10000); //PCM RAM
	

	//CDD & CDC Data
		//CDD
			unsigned int CDD_Data[8]; //makes an array for reading CDD unsigned int Data into
			for (j = 0; j < 8; j++) {
				ImportData(&CDD_Data[j], Data, 0xD1000  + (4 * j), 4);
			}
			for(i = 0; i < 10; i++) CDD.Rcv_Status[i] = Data[0xD1020 + i];
			for(i = 0; i < 10; i++) CDD.Trans_Comm[i] = Data[0xD102A + i];
			CDD.Fader = CDD_Data[0];
			CDD.Control = CDD_Data[1];
			CDD.Cur_Comm = CDD_Data[2];
			CDD.Status = CDD_Data[3];
			CDD.Minute = CDD_Data[4];
			CDD.Seconde = CDD_Data[5];
			CDD.Frame = CDD_Data[6];
			CDD.Ext = CDD_Data[7];
			if (CDD.Status & PLAYING) int unused = Resume_CDD_c7();
		//CDD end

		//CDC
			ImportData(&CDC.RS0, Data, 0xD1034, 4);
			ImportData(&CDC.RS1, Data, 0xD1038, 4);
			ImportData(&CDC.Host_Data, Data, 0xD103C, 4);
			ImportData(&CDC.DMA_Adr, Data, 0xD1040, 4);
			ImportData(&CDC.Stop_Watch, Data, 0xD1044, 4);
			ImportData(&CDC.COMIN, Data, 0xD1048, 4);
			ImportData(&CDC.IFSTAT, Data, 0xD104C, 4);
			ImportData(&CDC.DBC.N, Data, 0xD1050, 4);
			ImportData(&CDC.DAC.N, Data, 0xD1054, 4);
			ImportData(&CDC.HEAD.N, Data, 0xD1058, 4);
			ImportData(&CDC.PT.N, Data, 0xD105C, 4);
			ImportData(&CDC.WA.N, Data, 0xD1060, 4);
			ImportData(&CDC.STAT.N, Data, 0xD1064, 4);
			ImportData(&CDC.SBOUT, Data, 0xD1068, 4);
			ImportData(&CDC.IFCTRL, Data, 0xD106C, 4);
			ImportData(&CDC.CTRL.N, Data, 0xD1070, 4);
			ImportData(CDC.Buffer, Data, 0xD1074, ((32 * 1024 * 2) + 2352)); //Modif N. - added the *2 because the buffer appears to be that large
		//CDC end
	//CDD & CDC Data end

//		//Modif N. - seems like we should be saving these...?
//		unsigned int offset = 0xE19A4;
//		ImportDataAuto(&File_Add_Delay, Data, offset, 4);
////		ImportDataAuto(&CDDA_Enable, Data, offset, 4);
//		ImportDataAuto(CD_Audio_Buffer_L, Data, offset, 4*8192);
//		ImportDataAuto(CD_Audio_Buffer_R, Data, offset, 4*8192);
//		ImportDataAuto(&CD_Audio_Buffer_Read_Pos, Data, offset, 4);
//		ImportDataAuto(&CD_Audio_Buffer_Write_Pos, Data, offset, 4);
//		ImportDataAuto(&CD_Audio_Starting, Data, offset, 4);
//		ImportDataAuto(&CD_Present, Data, offset, 4);
//		ImportDataAuto(&CD_Load_System, Data, offset, 4);
//		ImportDataAuto(&CD_Timer_Counter, Data, offset, 4);
//		ImportDataAuto(&CDD_Complete, Data, offset, 4);
//		ImportDataAuto(&track_number, Data, offset, 4);
//		ImportDataAuto(&CD_timer_st, Data, offset, 4);
//		ImportDataAuto(&CD_LBA_st, Data, offset, 4);
//		ImportDataAuto(&SCD.TOC.First_Track, Data, offset, 1);
//		ImportDataAuto(&SCD.TOC.Last_Track, Data, offset, 1);
//		for(int i = 0; i < 100; i++)
//		{
//			ImportDataAuto(&SCD.TOC.Tracks[i].Type, Data, offset, 1);
//			ImportDataAuto(&SCD.TOC.Tracks[i].Num, Data, offset, 1);
//			ImportDataAuto(&SCD.TOC.Tracks[i].MSF.M, Data, offset, 1);
//			ImportDataAuto(&SCD.TOC.Tracks[i].MSF.S, Data, offset, 1);
//			ImportDataAuto(&SCD.TOC.Tracks[i].MSF.F, Data, offset, 1);
//		}
//		ImportDataAuto(&CDC_Decode_Reg_Read, Data, offset, 4);
}

void Export_SegaCD(unsigned char *Data) 
{
	S68000CONTEXT Context_sub68K;
	unsigned char *src;
	int i,j;

	sub68k_GetContext(&Context_sub68K);

	//sub68K bit goes here
		src = (unsigned char *) &(Context_sub68K.dreg[0]);
		for(i = 0; i < 8 * 4; i++) Data[i] = *src++;
		ExportData(&Context_sub68K.areg[0], Data, 0x20, 8 * 4);

		ExportData(&Context_sub68K.pc, Data, 0x48, 4);

		ExportData(&Context_sub68K.sr, Data, 0x50, 2);

		if(Context_sub68K.sr & 0x2000)
		{
			ExportData(&Context_sub68K.asp, Data, 0x52, 4);
			ExportData(&Context_sub68K.areg[7], Data, 0x56, 4);
		}
		else
		{
			ExportData(&Context_sub68K.asp, Data, 0x56, 4);
			ExportData(&Context_sub68K.areg[7], Data, 0x52, 4);
		}
		ExportData(&Context_sub68K.odometer, Data, 0x5A, 4);

		for(i = 0; i < 8; i++) Data[0x60 + i] = Context_sub68K.interrupts[i];

		ExportData(&Ram_Word_State, Data, 0x6C, 4);

	//here ends sub68k bit

	//PCM Chip dump
		ExportData(&PCM_Chip.Rate, Data, 0x100, 4);
		ExportData(&PCM_Chip.Enable, Data, 0x104, 4);
		ExportData(&PCM_Chip.Cur_Chan, Data, 0x108, 4);
		ExportData(&PCM_Chip.Bank, Data, 0x10C, 4);

		for (j = 0; j < 8; j++)
		{
			ExportData(&PCM_Chip.Channel[j].ENV, Data, 0x120 + (j * 4 * 11), 4);
			ExportData(&PCM_Chip.Channel[j].PAN, Data, 0x124 + (j * 4 * 11), 4);
			ExportData(&PCM_Chip.Channel[j].MUL_L, Data, 0x128 + (j * 4 * 11), 4);
			ExportData(&PCM_Chip.Channel[j].MUL_R, Data, 0x12C + (j * 4 * 11), 4);
			ExportData(&PCM_Chip.Channel[j].St_Addr, Data, 0x130 + (j * 4 * 11), 4);
			ExportData(&PCM_Chip.Channel[j].Loop_Addr, Data, 0x134 + (j * 4 * 11), 4);
			ExportData(&PCM_Chip.Channel[j].Addr, Data, 0x138 + (j * 4 * 11), 4);
			ExportData(&PCM_Chip.Channel[j].Step, Data, 0x13C + (j * 4 * 11), 4);
			ExportData(&PCM_Chip.Channel[j].Step_B, Data, 0x140 + (j * 4 * 11), 4);
			ExportData(&PCM_Chip.Channel[j].Enable, Data, 0x144 + (j * 4 * 11), 4);
			ExportData(&PCM_Chip.Channel[j].Data, Data, 0x148 + (j * 4 * 11), 4);
		}
	//End PCM Chip Dump

	//GFX State Dump
		ExportData(&Rot_Comp.Stamp_Size, Data, 0x300, 4);
		ExportData(&Rot_Comp.Stamp_Map_Adr, Data, 0x304, 4);
		ExportData(&Rot_Comp.IB_V_Cell_Size, Data, 0x308, 4);
		ExportData(&Rot_Comp.IB_Adr, Data, 0x30C, 4);
		ExportData(&Rot_Comp.IB_Offset, Data, 0x310, 4);
		ExportData(&Rot_Comp.IB_H_Dot_Size, Data, 0x314, 4);
		ExportData(&Rot_Comp.IB_V_Dot_Size, Data, 0x318, 4);
		ExportData(&Rot_Comp.Vector_Adr, Data, 0x31C, 4);
		ExportData(&Rot_Comp.Rotation_Running, Data, 0x320, 4);
	//End GFX State Dump

	//gate array bit
		ExportData(&COMM.Flag, Data, 0x0500, 4);
		src = (unsigned char *) &COMM.Command;
		for (j = 0; j < 8 * 2; j+=2) for (i = 0; i < 2; i++) Data[i + 0x0504 + j] = *src++;
		src = (unsigned char *) &COMM.Status;
		for (j = 0; j < 8 * 2; j+=2) for (i = 0; i < 2; i++) Data[i + 0x0514 + j] = *src++;
		ExportData(&Memory_Control_Status, Data, 0x0524, 4);
		ExportData(&Init_Timer_INT3, Data, 0x0528, 4);
		ExportData(&Timer_INT3, Data, 0x052C, 4);
		ExportData(&Timer_Step, Data, 0x0530, 4);
		ExportData(&Int_Mask_S68K, Data, 0x0534, 4);
		ExportData(&Font_COLOR, Data, 0x0538, 4);
		ExportData(&Font_BITS, Data, 0x053C, 4);
		ExportData(&CD_Access_Timer, Data, 0x0540, 4);
		ExportData(&SCD.Status_CDC, Data, 0x0544, 4);
		ExportData(&SCD.Status_CDD, Data, 0x0548, 4);
		ExportData(&SCD.Cur_LBA, Data, 0x054C, 4);
		ExportData(&SCD.Cur_Track, Data, 0x0550, 4);
		ExportData(&S68K_Mem_WP, Data, 0x0554, 4);
		ExportData(&S68K_Mem_PM, Data, 0x0558, 4);
		// More goes here When found
	//here ends gate array bit

	//Misc Status Flags
		ExportData(&Ram_Word_State, Data, 0xF00, 4); //For determining 1M or 2M
		ExportData(&LED_Status, Data, 0xF08, 4); //So the LED shows up properly
	//Word RAM state

	ExportData(Ram_Prg, Data, 0x1000, 0x80000); //Prg RAM
	
	//Word RAM
		if (Ram_Word_State >= 2) ExportData(Ram_Word_1M, Data, 0x81000, 0x40000); //1M mode
		else ExportData(Ram_Word_2M, Data, 0x81000, 0x40000); //2M mode
	//Word RAM end

	ExportData(Ram_PCM, Data, 0xC1000, 0x10000); //PCM RAM

	//CDD & CDC Data
		//CDD
			unsigned int CDD_src[8] = {CDD.Fader,CDD.Control,CDD.Cur_Comm,CDD.Status,CDD.Minute,CDD.Seconde,CDD.Frame,CDD.Ext}; // Makes an array for easier loop construction
			for (j = 0; j < 8; j++) {
				ExportData(&CDD_src[j], Data, 0xD1000  + (4 * j), 4);
			}
			for(i = 0; i < 10; i++) Data[0xD1020 + i] = CDD.Rcv_Status[i];
			for(i = 0; i < 10; i++) Data[0xD102A + i] = CDD.Trans_Comm[i];
		//CDD end

		//CDC
			ExportData(&CDC.RS0, Data, 0xD1034, 4);
			ExportData(&CDC.RS1, Data, 0xD1038, 4);
			ExportData(&CDC.Host_Data, Data, 0xD103C, 4);
			ExportData(&CDC.DMA_Adr, Data, 0xD1040, 4);
			ExportData(&CDC.Stop_Watch, Data, 0xD1044, 4);
			ExportData(&CDC.COMIN, Data, 0xD1048, 4);
			ExportData(&CDC.IFSTAT, Data, 0xD104C, 4);
			ExportData(&CDC.DBC.N, Data, 0xD1050, 4);
			ExportData(&CDC.DAC.N, Data, 0xD1054, 4);
			ExportData(&CDC.HEAD.N, Data, 0xD1058, 4);
			ExportData(&CDC.PT.N, Data, 0xD105C, 4);
			ExportData(&CDC.WA.N, Data, 0xD1060, 4);
			ExportData(&CDC.STAT.N, Data, 0xD1064, 4);
			ExportData(&CDC.SBOUT, Data, 0xD1068, 4);
			ExportData(&CDC.IFCTRL, Data, 0xD106C, 4);
			ExportData(&CDC.CTRL.N, Data, 0xD1070, 4);
			ExportData(CDC.Buffer, Data, 0xD1074, ((32 * 1024 * 2) + 2352)); //Modif N. - added the *2 because the buffer appears to be that large
		//CDC end
	//CDD & CDC Data end

//		//Modif N. - seems like we should be saving these...?
//		unsigned int offset = 0xE19A4;
//		ExportDataAuto(&File_Add_Delay, Data, offset, 4);
////		ExportDataAuto(&CDDA_Enable, Data, offset, 4);
//		ExportDataAuto(CD_Audio_Buffer_L, Data, offset, 4*8192);
//		ExportDataAuto(CD_Audio_Buffer_R, Data, offset, 4*8192);
//		ExportDataAuto(&CD_Audio_Buffer_Read_Pos, Data, offset, 4);
//		ExportDataAuto(&CD_Audio_Buffer_Write_Pos, Data, offset, 4);
//		ExportDataAuto(&CD_Audio_Starting, Data, offset, 4);
//		ExportDataAuto(&CD_Present, Data, offset, 4);
//		ExportDataAuto(&CD_Load_System, Data, offset, 4);
//		ExportDataAuto(&CD_Timer_Counter, Data, offset, 4);
//		ExportDataAuto(&CDD_Complete, Data, offset, 4);
//		ExportDataAuto(&track_number, Data, offset, 4);
//		ExportDataAuto(&CD_timer_st, Data, offset, 4);
//		ExportDataAuto(&CD_LBA_st, Data, offset, 4);
//		ExportDataAuto(&SCD.TOC.First_Track, Data, offset, 1);
//		ExportDataAuto(&SCD.TOC.Last_Track, Data, offset, 1);
//		for(int i = 0; i < 100; i++)
//		{
//			ExportDataAuto(&SCD.TOC.Tracks[i].Type, Data, offset, 1);
//			ExportDataAuto(&SCD.TOC.Tracks[i].Num, Data, offset, 1);
//			ExportDataAuto(&SCD.TOC.Tracks[i].MSF.M, Data, offset, 1);
//			ExportDataAuto(&SCD.TOC.Tracks[i].MSF.S, Data, offset, 1);
//			ExportDataAuto(&SCD.TOC.Tracks[i].MSF.F, Data, offset, 1);
//		}
//		ExportDataAuto(&CDC_Decode_Reg_Read, Data, offset, 4);
}

void Import_32X(unsigned char *Data)
{
//	unsigned char *src;
	int i;

	for(i = 0; i < 0x1000; i++) M_SH2.Cache[i] = Data[i];
	for(i = 0; i < 0x200; i++) SH2_Write_Byte(&M_SH2, 0xFFFFFE00 + i, Data[0x1000 + i]);

	ImportData(&(M_SH2.R[0]), Data, 0x1200, 16 * 4);

	SH2_Set_SR(&M_SH2, (Data[0x1243] << 24) + (Data[0x1242] << 16) + (Data[0x1241] << 8) + Data[0x1240]);
	SH2_Set_GBR(&M_SH2, (Data[0x1247] << 24) + (Data[0x1246] << 16) + (Data[0x1245] << 8) + Data[0x1244]);
	SH2_Set_VBR(&M_SH2, (Data[0x124B] << 24) + (Data[0x124A] << 16) + (Data[0x1249] << 8) + Data[0x1248]);
	SH2_Set_MACL(&M_SH2, (Data[0x124F] << 24) + (Data[0x124E] << 16) + (Data[0x124D] << 8) + Data[0x124C]);
	SH2_Set_MACH(&M_SH2, (Data[0x1253] << 24) + (Data[0x1252] << 16) + (Data[0x1251] << 8) + Data[0x1250]);
	SH2_Set_PR(&M_SH2, (Data[0x1257] << 24) + (Data[0x1256] << 16) + (Data[0x1255] << 8) + Data[0x1254]);
	SH2_Set_PC(&M_SH2, (Data[0x125B] << 24) + (Data[0x125A] << 16) + (Data[0x1259] << 8) + Data[0x1258]);
	M_SH2.Status = (Data[0x125F] << 24) + (Data[0x125E] << 16) + (Data[0x125D] << 8) + Data[0x125C];

	for(i = 0; i < 0x1000; i++) S_SH2.Cache[i] = Data[i + 0x1400];
	for(i = 0; i < 0x200; i++) SH2_Write_Byte(&S_SH2, 0xFFFFFE00 + i, Data[0x2400 + i]);

	ImportData(&(S_SH2.R[0]), Data, 0x2600, 16 * 4);

	SH2_Set_SR(&S_SH2, (Data[0x2643] << 24) + (Data[0x2642] << 16) + (Data[0x2641] << 8) + Data[0x2640]);
	SH2_Set_GBR(&S_SH2, (Data[0x2647] << 24) + (Data[0x2646] << 16) + (Data[0x2645] << 8) + Data[0x2644]);
	SH2_Set_VBR(&S_SH2, (Data[0x264B] << 24) + (Data[0x264A] << 16) + (Data[0x2649] << 8) + Data[0x2648]);
	SH2_Set_MACL(&S_SH2, (Data[0x264F] << 24) + (Data[0x264E] << 16) + (Data[0x264D] << 8) + Data[0x264C]);
	SH2_Set_MACH(&S_SH2, (Data[0x2653] << 24) + (Data[0x2652] << 16) + (Data[0x2651] << 8) + Data[0x2650]);
	SH2_Set_PR(&S_SH2, (Data[0x2657] << 24) + (Data[0x2656] << 16) + (Data[0x2655] << 8) + Data[0x2654]);
	SH2_Set_PC(&S_SH2, (Data[0x265B] << 24) + (Data[0x265A] << 16) + (Data[0x2659] << 8) + Data[0x2658]);
	S_SH2.Status = (Data[0x265F] << 24) + (Data[0x265E] << 16) + (Data[0x265D] << 8) + Data[0x265C];

	_32X_ADEN = Data[0x2700] & 1;
	_32X_RV = Data[0x2705] & 1;
	M68K_32X_Mode();
	_32X_FM = Data[0x2701] & 0x80;
	_32X_Set_FB();
	_32X_MINT = Data[0x2702];
	_32X_SINT = Data[0x2703];
	Bank_SH2 = Data[0x2704];
	M68K_Set_32X_Rom_Bank();
	
	/*******
	FIFO stuff to add here...	
	 *******/
	for(i = 0; i < 0x10; i++) SH2_Write_Byte(&M_SH2, 0x4020 + i, Data[0x2740 + i]);
	for(i = 0; i < 4; i++) SH2_Write_Byte(&M_SH2, 0x4030 + i, Data[0x2750 + i]);
	/*******
	Extra PWM stuff to add here...	
	 *******/

	// Do it to allow VDP write on 32X side
	_32X_FM = 0x80;
	_32X_Set_FB();

	for(i = 0; i < 0x10; i++) SH2_Write_Byte(&M_SH2, 0x4100 + i, Data[0x2760 + i]);
	for(i = 0; i < 0x200; i += 2) SH2_Write_Word(&M_SH2, 0x4200 + i, Data[i + 0x2800 + 0] + (Data[i + 0x2800 + 1] << 8));
	for(i = 0; i < 0x40000; i++) SH2_Write_Byte(&M_SH2, 0x6000000 + i, Data[0x2A00 + i]);

	_32X_FM = Data[0x2701] & 0x80;
	_32X_Set_FB();

	if (SH2_Read_Word(&M_SH2, 0x410A) & 1)
	{
		ImportData(_32X_VDP_Ram, Data, 0x42A00, 0x20000);
		for(i = 0; i < 0x20000; i++) _32X_VDP_Ram[i + 0x20000] = Data[0x62A00 + i];
	}
	else
	{
		for(i = 0; i < 0x20000; i++) _32X_VDP_Ram[i + 0x20000] = Data[0x42A00 + i];
		ImportData(_32X_VDP_Ram, Data, 0x62A00, 0x20000);
	}
}


void Export_32X(unsigned char *Data)
{
//	unsigned char *src;
	int i;

	for(i = 0; i < 0x1000; i++) Data[i] = M_SH2.Cache[i];
	for(i = 0; i < 0x200; i++) Data[0x1000 + i] = M_SH2.IO_Reg[i];

	ExportData(&M_SH2.R[0], Data, 0x1200, 16 * 4);

	i = SH2_Get_SR(&M_SH2);
	Data[0x1240] = (unsigned char) (i & 0xFF);
	Data[0x1241] = (unsigned char) ((i >> 8) & 0xFF);
	Data[0x1242] = (unsigned char) ((i >> 16) & 0xFF);
	Data[0x1243] = (unsigned char) ((i >> 24) & 0xFF);
	i = SH2_Get_GBR(&M_SH2);
	Data[0x1244] = (unsigned char) (i & 0xFF);
	Data[0x1245] = (unsigned char) ((i >> 8) & 0xFF);
	Data[0x1246] = (unsigned char) ((i >> 16) & 0xFF);
	Data[0x1247] = (unsigned char) ((i >> 24) & 0xFF);
	i = SH2_Get_VBR(&M_SH2);
	Data[0x1248] = (unsigned char) (i & 0xFF);
	Data[0x1249] = (unsigned char) ((i >> 8) & 0xFF);
	Data[0x124A] = (unsigned char) ((i >> 16) & 0xFF);
	Data[0x124B] = (unsigned char) ((i >> 24) & 0xFF);
	i = SH2_Get_MACL(&M_SH2);
	Data[0x124C] = (unsigned char) (i & 0xFF);
	Data[0x124D] = (unsigned char) ((i >> 8) & 0xFF);
	Data[0x124E] = (unsigned char) ((i >> 16) & 0xFF);
	Data[0x124F] = (unsigned char) ((i >> 24) & 0xFF);
	i = SH2_Get_MACH(&M_SH2);
	Data[0x1250] = (unsigned char) (i & 0xFF);
	Data[0x1251] = (unsigned char) ((i >> 8) & 0xFF);
	Data[0x1252] = (unsigned char) ((i >> 16) & 0xFF);
	Data[0x1253] = (unsigned char) ((i >> 24) & 0xFF);
	i = SH2_Get_PR(&M_SH2);
	Data[0x1254] = (unsigned char) (i & 0xFF);
	Data[0x1255] = (unsigned char) ((i >> 8) & 0xFF);
	Data[0x1256] = (unsigned char) ((i >> 16) & 0xFF);
	Data[0x1257] = (unsigned char) ((i >> 24) & 0xFF);
	i = SH2_Get_PC(&M_SH2);
	Data[0x1258] = (unsigned char) (i & 0xFF);
	Data[0x1259] = (unsigned char) ((i >> 8) & 0xFF);
	Data[0x125A] = (unsigned char) ((i >> 16) & 0xFF);
	Data[0x125B] = (unsigned char) ((i >> 24) & 0xFF);
	i = M_SH2.Status;
	Data[0x125C] = (unsigned char) (i & 0xFF);
	Data[0x125D] = (unsigned char) ((i >> 8) & 0xFF);
	Data[0x125E] = (unsigned char) ((i >> 16) & 0xFF);
	Data[0x125F] = (unsigned char) ((i >> 24) & 0xFF);

	for(i = 0; i < 0x1000; i++) Data[0x1400 + i] = S_SH2.Cache[i];
	for(i = 0; i < 0x200; i++) Data[0x2400 + i] = S_SH2.IO_Reg[i];

	ExportData(&S_SH2.R[0], Data, 0x2600, 16 * 4);

	i = SH2_Get_SR(&S_SH2);
	Data[0x2640] = (unsigned char) (i & 0xFF);
	Data[0x2641] = (unsigned char) ((i >> 8) & 0xFF);
	Data[0x2642] = (unsigned char) ((i >> 16) & 0xFF);
	Data[0x2643] = (unsigned char) ((i >> 24) & 0xFF);
	i = SH2_Get_GBR(&S_SH2);
	Data[0x2644] = (unsigned char) (i & 0xFF);
	Data[0x2645] = (unsigned char) ((i >> 8) & 0xFF);
	Data[0x2646] = (unsigned char) ((i >> 16) & 0xFF);
	Data[0x2647] = (unsigned char) ((i >> 24) & 0xFF);
	i = SH2_Get_VBR(&S_SH2);
	Data[0x2648] = (unsigned char) (i & 0xFF);
	Data[0x2649] = (unsigned char) ((i >> 8) & 0xFF);
	Data[0x264A] = (unsigned char) ((i >> 16) & 0xFF);
	Data[0x264B] = (unsigned char) ((i >> 24) & 0xFF);
	i = SH2_Get_MACL(&S_SH2);
	Data[0x264C] = (unsigned char) (i & 0xFF);
	Data[0x264D] = (unsigned char) ((i >> 8) & 0xFF);
	Data[0x264E] = (unsigned char) ((i >> 16) & 0xFF);
	Data[0x264F] = (unsigned char) ((i >> 24) & 0xFF);
	i = SH2_Get_MACH(&S_SH2);
	Data[0x2650] = (unsigned char) (i & 0xFF);
	Data[0x2651] = (unsigned char) ((i >> 8) & 0xFF);
	Data[0x2652] = (unsigned char) ((i >> 16) & 0xFF);
	Data[0x2653] = (unsigned char) ((i >> 24) & 0xFF);
	i = SH2_Get_PR(&S_SH2);
	Data[0x2654] = (unsigned char) (i & 0xFF);
	Data[0x2655] = (unsigned char) ((i >> 8) & 0xFF);
	Data[0x2656] = (unsigned char) ((i >> 16) & 0xFF);
	Data[0x2657] = (unsigned char) ((i >> 24) & 0xFF);
	i = SH2_Get_PC(&S_SH2);
	Data[0x2658] = (unsigned char) (i & 0xFF);
	Data[0x2659] = (unsigned char) ((i >> 8) & 0xFF);
	Data[0x265A] = (unsigned char) ((i >> 16) & 0xFF);
	Data[0x265B] = (unsigned char) ((i >> 24) & 0xFF);
	i = S_SH2.Status;
	Data[0x265C] = (unsigned char) (i & 0xFF);
	Data[0x265D] = (unsigned char) ((i >> 8) & 0xFF);
	Data[0x265E] = (unsigned char) ((i >> 16) & 0xFF);
	Data[0x265F] = (unsigned char) ((i >> 24) & 0xFF);

	// Do it to allow VDP write on 32X side
	Data[0x2700] = _32X_ADEN & 1;
	Data[0x2705] = _32X_RV & 1;
	Data[0x2701] = _32X_FM & 0x80;
	Data[0x2702] = _32X_MINT;
	Data[0x2703] = _32X_SINT;
	Data[0x2704] = Bank_SH2;
	
	/*******
	FIFO stuff to add here...	
	 *******/
	for(i = 0; i < 0x10; i++) Data[0x2740 + i] = SH2_Read_Byte(&M_SH2, 0x4020 + i);
	for(i = 0; i < 4; i++) Data[0x2750 + i] = SH2_Read_Byte(&M_SH2, 0x4030 + i);
	/*******
	Extra PWM stuff to add here...	
	 *******/

	// Do it to allow VDP write on 32X side
	_32X_FM = 0x80;
	_32X_Set_FB();

	for(i = 0; i < 0x10; i++) Data[0x2760 + i] = SH2_Read_Byte(&M_SH2, 0x4100 + i);
	for(i = 0; i < 0x200; i += 2)
	{
		Data[i + 0x2800 + 0] = (unsigned char) (SH2_Read_Word(&M_SH2, 0x4200 + i) & 0xFF);
		Data[i + 0x2800 + 1] = (unsigned char) (SH2_Read_Word(&M_SH2, 0x4200 + i) >> 8);
	}
	for(i = 0; i < 0x40000; i++) Data[0x2A00 + i] = SH2_Read_Byte(&M_SH2, 0x6000000 + i);

	_32X_FM = Data[0x2701] & 0x80;
	_32X_Set_FB();

	if (SH2_Read_Word(&M_SH2, 0x410A) & 1)
	{
		ExportData(_32X_VDP_Ram, Data, 0x42A00, 0x20000);
		for(i = 0; i < 0x20000; i++) Data[0x62A00 + i] = _32X_VDP_Ram[i + 0x20000];
	}
	else
	{
		for(i = 0; i < 0x20000; i++) Data[0x42A00 + i] = _32X_VDP_Ram[i + 0x20000];
		ExportData(_32X_VDP_Ram, Data, 0x62A00, 0x20000);
	}
}


int Save_Config(char *File_Name)
{
	char Conf_File[1024];

	strcpy(Conf_File, File_Name);

	WritePrivateProfileString("General", "Rom path", Rom_Dir, Conf_File);
	WritePrivateProfileString("General", "Save path", State_Dir, Conf_File);
	WritePrivateProfileString("General", "SRAM path", SRAM_Dir, Conf_File);
	WritePrivateProfileString("General", "BRAM path", BRAM_Dir, Conf_File);
	WritePrivateProfileString("General", "Dump path", Dump_Dir, Conf_File);
	WritePrivateProfileString("General", "Dump GYM path", Dump_GYM_Dir, Conf_File);
	WritePrivateProfileString("General", "Screen Shot path", ScrShot_Dir, Conf_File);
	WritePrivateProfileString("General", "Patch path", Patch_Dir, Conf_File);
	WritePrivateProfileString("General", "IPS Patch path", IPS_Dir, Conf_File);
	WritePrivateProfileString("General", "Movie path", Movie_Dir, Conf_File);
	
	WritePrivateProfileString("General", "Genesis Bios", Genesis_Bios, Conf_File);

	WritePrivateProfileString("General", "USA CD Bios", US_CD_Bios, Conf_File);
	WritePrivateProfileString("General", "EUROPE CD Bios", EU_CD_Bios, Conf_File);
	WritePrivateProfileString("General", "JAPAN CD Bios", JA_CD_Bios, Conf_File);

	WritePrivateProfileString("General", "32X 68000 Bios", _32X_Genesis_Bios, Conf_File);
	WritePrivateProfileString("General", "32X Master SH2 Bios", _32X_Master_Bios, Conf_File);
	WritePrivateProfileString("General", "32X Slave SH2 Bios", _32X_Slave_Bios, Conf_File);

	WritePrivateProfileString("General", "Rom 1", Recent_Rom[0], Conf_File);
	WritePrivateProfileString("General", "Rom 2", Recent_Rom[1], Conf_File);
	WritePrivateProfileString("General", "Rom 3", Recent_Rom[2], Conf_File);
	WritePrivateProfileString("General", "Rom 4", Recent_Rom[3], Conf_File);
	WritePrivateProfileString("General", "Rom 5", Recent_Rom[4], Conf_File);
	WritePrivateProfileString("General", "Rom 6", Recent_Rom[5], Conf_File);
	WritePrivateProfileString("General", "Rom 7", Recent_Rom[6], Conf_File);
	WritePrivateProfileString("General", "Rom 8", Recent_Rom[7], Conf_File);
	WritePrivateProfileString("General", "Rom 9", Recent_Rom[8], Conf_File);
	WritePrivateProfileString("General", "Rom 10", Recent_Rom[9], Conf_File);
	WritePrivateProfileString("General", "Rom 11", Recent_Rom[10], Conf_File);
	WritePrivateProfileString("General", "Rom 12", Recent_Rom[11], Conf_File);
	WritePrivateProfileString("General", "Rom 13", Recent_Rom[12], Conf_File);
	WritePrivateProfileString("General", "Rom 14", Recent_Rom[13], Conf_File);
	WritePrivateProfileString("General", "Rom 15", Recent_Rom[14], Conf_File);

	wsprintf(Str_Tmp, "%d", File_Type_Index);
	WritePrivateProfileString("General", "File type index", Str_Tmp, Conf_File);

	wsprintf(Str_Tmp, "%d", Current_State);
	WritePrivateProfileString("General", "State Number", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Language);
	WritePrivateProfileString("General", "Language", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Window_Pos.x);
	WritePrivateProfileString("General", "Window X", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Window_Pos.y);
	WritePrivateProfileString("General", "Window Y", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Intro_Style); //Modif N. - Save intro style to config, instead of only reading it from there
	WritePrivateProfileString("General", "Intro Style", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Effect_Color);
	WritePrivateProfileString("General", "Free Mode Color", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Def_Read_Only); //Upth-Add - Save the default read only flag to config
	WritePrivateProfileString("General", "Movie Default Read Only", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", AutoCloseMovie); //Upth-Add - Save the auto close movie flag to config
	WritePrivateProfileString("General", "Movie Auto Close", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", UseMovieStates); //Upth-Add - Save the auto close movie flag to config
	WritePrivateProfileString("General", "Movie Based State Names", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", SlowDownSpeed); //Upth-Add - Save the current slowdown speed to config
	WritePrivateProfileString("General", "Slow Down Speed", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", DelayFactor); //Upth-Add - Make frame advance speed configurable
	WritePrivateProfileString("General", "Frame Advance Delay Factor", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Sleep_Time); //Modif N. - CPU hogging now a real setting
	WritePrivateProfileString("General", "Allow Idle", Str_Tmp, Conf_File);

	wsprintf(Str_Tmp, "%d", Full_Screen & 1);
	WritePrivateProfileString("Graphics", "Full Screen", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", FS_VSync & 1);
	WritePrivateProfileString("Graphics", "Full Screen VSync", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Res_X); //Upth-Add - Save our full screen X resolution to config
	WritePrivateProfileString("Graphics", "Full Screen X Resolution", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Res_Y); //Upth-Add - Save our full screen Y resolution to config
	WritePrivateProfileString("Graphics", "Full Screen Y Resolution", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", FS_No_Res_Change); //Upth-Add - Save our fullscreen without resolution change flag
	WritePrivateProfileString("Graphics", "Full Screen No Res Change", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", W_VSync & 1);
	WritePrivateProfileString("Graphics", "Windows VSync", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Render_W);
	WritePrivateProfileString("Graphics", "Render Windowed", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Render_FS);
	WritePrivateProfileString("Graphics", "Render Fullscreen", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Stretch & 1);
	WritePrivateProfileString("Graphics", "Stretch", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Blit_Soft & 1);
	WritePrivateProfileString("Graphics", "Software Blit", Str_Tmp, Conf_File);

	wsprintf(Str_Tmp, "%d", Contrast_Level);
	WritePrivateProfileString("Graphics", "Contrast", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Brightness_Level);
	WritePrivateProfileString("Graphics", "Brightness", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Greyscale & 1);
	WritePrivateProfileString("Graphics", "Greyscale", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Invert_Color & 1);
	WritePrivateProfileString("Graphics", "Invert", Str_Tmp, Conf_File);

	wsprintf(Str_Tmp, "%d", Sprite_Over & 1);
	WritePrivateProfileString("Graphics", "Sprite limit", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Frame_Skip);
	WritePrivateProfileString("Graphics", "Frame skip", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", CleanAvi);
	WritePrivateProfileString("Graphics", "Clean Avi", Str_Tmp, Conf_File);

	wsprintf(Str_Tmp, "%d", Sound_Enable & 1);
	WritePrivateProfileString("Sound", "State", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Sound_Rate);
	WritePrivateProfileString("Sound", "Rate", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Sound_Stereo);
	WritePrivateProfileString("Sound", "Stereo", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Sound_Soften); // Modif N.
	WritePrivateProfileString("Sound", "SoundSoftenFilter", Str_Tmp, Conf_File); // Modif N.
	wsprintf(Str_Tmp, "%d", Z80_State & 1);
	WritePrivateProfileString("Sound", "Z80 State", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", YM2612_Enable & 1);
	WritePrivateProfileString("Sound", "YM2612 State", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", PSG_Enable & 1);
	WritePrivateProfileString("Sound", "PSG State", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", DAC_Enable & 1);
	WritePrivateProfileString("Sound", "DAC State", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", PCM_Enable & 1);
	WritePrivateProfileString("Sound", "PCM State", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", PWM_Enable & 1);
	WritePrivateProfileString("Sound", "PWM State", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", CDDA_Enable & 1);
	WritePrivateProfileString("Sound", "CDDA State", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", YM2612_Improv & 1);
	WritePrivateProfileString("Sound", "YM2612 Improvement", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", DAC_Improv & 1);
	WritePrivateProfileString("Sound", "DAC Improvement", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", PSG_Improv & 1);
	WritePrivateProfileString("Sound", "PSG Improvement", Str_Tmp, Conf_File);

	wsprintf(Str_Tmp,"%d",MastVol);
	WritePrivateProfileString("Sound", "Master Volume", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp,"%d",YM2612Vol);
	WritePrivateProfileString("Sound", "YM2612 Volume", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp,"%d",DACVol);
	WritePrivateProfileString("Sound", "DAC Volume", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp,"%d",PSGVol);
	WritePrivateProfileString("Sound", "PSG Volume", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp,"%d",PCMVol);
	WritePrivateProfileString("Sound", "PCM Volume", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp,"%d",CDDAVol);
	WritePrivateProfileString("Sound", "CDDA Volume", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp,"%d",PWMVol);
	WritePrivateProfileString("Sound", "PWM Volume", Str_Tmp, Conf_File);


	wsprintf(Str_Tmp, "%d", Country);
	WritePrivateProfileString("CPU", "Country", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Country_Order[0]);
	WritePrivateProfileString("CPU", "Prefered Country 1", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Country_Order[1]);
	WritePrivateProfileString("CPU", "Prefered Country 2", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Country_Order[2]);
	WritePrivateProfileString("CPU", "Prefered Country 3", Str_Tmp, Conf_File);

	wsprintf(Str_Tmp, "%d", SegaCD_Accurate);
	WritePrivateProfileString("CPU", "Perfect synchro between main and sub CPU (Sega CD)", Str_Tmp, Conf_File);

	wsprintf(Str_Tmp, "%d", MSH2_Speed);
	WritePrivateProfileString("CPU", "Main SH2 Speed", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", SSH2_Speed);
	WritePrivateProfileString("CPU", "Slave SH2 Speed", Str_Tmp, Conf_File);

	wsprintf(Str_Tmp, "%d", Fast_Blur & 1);
	WritePrivateProfileString("Options", "Fast Blur", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Show_FPS & 1);
	WritePrivateProfileString("Options", "FPS", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", FPS_Style);
	WritePrivateProfileString("Options", "FPS Style", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Show_Message & 1);
	WritePrivateProfileString("Options", "Message", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Message_Style);
	WritePrivateProfileString("Options", "Message Style", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Show_LED & 1);
	WritePrivateProfileString("Options", "LED", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Auto_Fix_CS & 1);
	WritePrivateProfileString("Options", "Auto Fix Checksum", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Auto_Pause & 1);
	WritePrivateProfileString("Options", "Auto Pause", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", CUR_DEV);
	WritePrivateProfileString("Options", "CD Drive", Str_Tmp, Conf_File);

	if (BRAM_Ex_State & 0x100)
	{
		wsprintf(Str_Tmp, "%d", BRAM_Ex_Size);
		WritePrivateProfileString("Options", "Ram Cart Size", Str_Tmp, Conf_File);
	}
	else
	{
		WritePrivateProfileString("Options", "Ram Cart Size", "-1", Conf_File);
	}
	
	WritePrivateProfileString("Options", "GCOffline path", CGOffline_Path, Conf_File);
	WritePrivateProfileString("Options", "Gens manual path", Manual_Path, Conf_File);

	wsprintf(Str_Tmp, "%d", Disable_Blue_Screen);//Modif
	WritePrivateProfileString("Options", "Disable Blue Screen", Str_Tmp, Conf_File);//Modif
	wsprintf(Str_Tmp, "%d", FrameCounterEnabled);//Modif
	WritePrivateProfileString("Options", "FrameCounterEnabled", Str_Tmp, Conf_File);//Modif
	wsprintf(Str_Tmp, "%d", FrameCounterFrames);//Modif N.
	WritePrivateProfileString("Options", "FrameCounterFrames", Str_Tmp, Conf_File);//Modif N.
	wsprintf(Str_Tmp, "%d", FrameCounterPosition);//Modif
	WritePrivateProfileString("Options", "FrameCounterPosition", Str_Tmp, Conf_File);//Modif
	wsprintf(Str_Tmp, "%d", LagCounterEnabled);//Modif
	WritePrivateProfileString("Options", "LagCounterEnabled", Str_Tmp, Conf_File);//Modif
	wsprintf(Str_Tmp, "%d", LagCounterFrames);//Modif N.
	WritePrivateProfileString("Options", "LagCounterFrames", Str_Tmp, Conf_File);//Modif N.
	wsprintf(Str_Tmp, "%d", ShowInputEnabled);//Modif
	WritePrivateProfileString("Options", "ShowInputEnabled", Str_Tmp, Conf_File);//Modif
	wsprintf(Str_Tmp, "%d", AutoBackupEnabled);//Modif
	WritePrivateProfileString("Options", "AutoBackupEnabled", Str_Tmp, Conf_File);//Modif

	wsprintf(Str_Tmp, "%d", Controller_1_Type & 0x13);
	WritePrivateProfileString("Input", "P1.Type", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[0].Up);
	WritePrivateProfileString("Input", "P1.Up", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[0].Down);
	WritePrivateProfileString("Input", "P1.Down", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[0].Left);
	WritePrivateProfileString("Input", "P1.Left", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[0].Right);
	WritePrivateProfileString("Input", "P1.Right", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[0].Start);
	WritePrivateProfileString("Input", "P1.Start", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[0].A);
	WritePrivateProfileString("Input", "P1.A", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[0].B);
	WritePrivateProfileString("Input", "P1.B", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[0].C);
	WritePrivateProfileString("Input", "P1.C", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[0].Mode);
	WritePrivateProfileString("Input", "P1.Mode", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[0].X);
	WritePrivateProfileString("Input", "P1.X", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[0].Y);
	WritePrivateProfileString("Input", "P1.Y", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[0].Z);
	WritePrivateProfileString("Input", "P1.Z", Str_Tmp, Conf_File);

	wsprintf(Str_Tmp, "%d", Controller_1B_Type & 0x03);
	WritePrivateProfileString("Input", "P1B.Type", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[2].Up);
	WritePrivateProfileString("Input", "P1B.Up", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[2].Down);
	WritePrivateProfileString("Input", "P1B.Down", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[2].Left);
	WritePrivateProfileString("Input", "P1B.Left", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[2].Right);
	WritePrivateProfileString("Input", "P1B.Right", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[2].Start);
	WritePrivateProfileString("Input", "P1B.Start", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[2].A);
	WritePrivateProfileString("Input", "P1B.A", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[2].B);
	WritePrivateProfileString("Input", "P1B.B", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[2].C);
	WritePrivateProfileString("Input", "P1B.C", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[2].Mode);
	WritePrivateProfileString("Input", "P1B.Mode", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[2].X);
	WritePrivateProfileString("Input", "P1B.X", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[2].Y);
	WritePrivateProfileString("Input", "P1B.Y", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[2].Z);
	WritePrivateProfileString("Input", "P1B.Z", Str_Tmp, Conf_File);

	wsprintf(Str_Tmp, "%d", Controller_1C_Type & 0x03);
	WritePrivateProfileString("Input", "P1C.Type", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[3].Up);
	WritePrivateProfileString("Input", "P1C.Up", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[3].Down);
	WritePrivateProfileString("Input", "P1C.Down", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[3].Left);
	WritePrivateProfileString("Input", "P1C.Left", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[3].Right);
	WritePrivateProfileString("Input", "P1C.Right", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[3].Start);
	WritePrivateProfileString("Input", "P1C.Start", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[3].A);
	WritePrivateProfileString("Input", "P1C.A", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[3].B);
	WritePrivateProfileString("Input", "P1C.B", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[3].C);
	WritePrivateProfileString("Input", "P1C.C", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[3].Mode);
	WritePrivateProfileString("Input", "P1C.Mode", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[3].X);
	WritePrivateProfileString("Input", "P1C.X", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[3].Y);
	WritePrivateProfileString("Input", "P1C.Y", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[3].Z);
	WritePrivateProfileString("Input", "P1C.Z", Str_Tmp, Conf_File);

	wsprintf(Str_Tmp, "%d", Controller_1D_Type & 0x03);
	WritePrivateProfileString("Input", "P1D.Type", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[4].Up);
	WritePrivateProfileString("Input", "P1D.Up", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[4].Down);
	WritePrivateProfileString("Input", "P1D.Down", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[4].Left);
	WritePrivateProfileString("Input", "P1D.Left", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[4].Right);
	WritePrivateProfileString("Input", "P1D.Right", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[4].Start);
	WritePrivateProfileString("Input", "P1D.Start", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[4].A);
	WritePrivateProfileString("Input", "P1D.A", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[4].B);
	WritePrivateProfileString("Input", "P1D.B", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[4].C);
	WritePrivateProfileString("Input", "P1D.C", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[4].Mode);
	WritePrivateProfileString("Input", "P1D.Mode", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[4].X);
	WritePrivateProfileString("Input", "P1D.X", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[4].Y);
	WritePrivateProfileString("Input", "P1D.Y", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[4].Z);
	WritePrivateProfileString("Input", "P1D.Z", Str_Tmp, Conf_File);

	wsprintf(Str_Tmp, "%d", Controller_2_Type & 0x13);
	WritePrivateProfileString("Input", "P2.Type", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[1].Up);
	WritePrivateProfileString("Input", "P2.Up", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[1].Down);
	WritePrivateProfileString("Input", "P2.Down", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[1].Left);
	WritePrivateProfileString("Input", "P2.Left", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[1].Right);
	WritePrivateProfileString("Input", "P2.Right", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[1].Start);
	WritePrivateProfileString("Input", "P2.Start", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[1].A);
	WritePrivateProfileString("Input", "P2.A", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[1].B);
	WritePrivateProfileString("Input", "P2.B", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[1].C);
	WritePrivateProfileString("Input", "P2.C", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[1].Mode);
	WritePrivateProfileString("Input", "P2.Mode", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[1].X);
	WritePrivateProfileString("Input", "P2.X", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[1].Y);
	WritePrivateProfileString("Input", "P2.Y", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[1].Z);
	WritePrivateProfileString("Input", "P2.Z", Str_Tmp, Conf_File);

	wsprintf(Str_Tmp, "%d", Controller_2B_Type & 0x03);
	WritePrivateProfileString("Input", "P2B.Type", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[5].Up);
	WritePrivateProfileString("Input", "P2B.Up", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[5].Down);
	WritePrivateProfileString("Input", "P2B.Down", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[5].Left);
	WritePrivateProfileString("Input", "P2B.Left", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[5].Right);
	WritePrivateProfileString("Input", "P2B.Right", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[5].Start);
	WritePrivateProfileString("Input", "P2B.Start", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[5].A);
	WritePrivateProfileString("Input", "P2B.A", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[5].B);
	WritePrivateProfileString("Input", "P2B.B", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[5].C);
	WritePrivateProfileString("Input", "P2B.C", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[5].Mode);
	WritePrivateProfileString("Input", "P2B.Mode", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[5].X);
	WritePrivateProfileString("Input", "P2B.X", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[5].Y);
	WritePrivateProfileString("Input", "P2B.Y", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[5].Z);
	WritePrivateProfileString("Input", "P2B.Z", Str_Tmp, Conf_File);

	wsprintf(Str_Tmp, "%d", Controller_2C_Type & 0x03);
	WritePrivateProfileString("Input", "P2C.Type", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[6].Up);
	WritePrivateProfileString("Input", "P2C.Up", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[6].Down);
	WritePrivateProfileString("Input", "P2C.Down", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[6].Left);
	WritePrivateProfileString("Input", "P2C.Left", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[6].Right);
	WritePrivateProfileString("Input", "P2C.Right", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[6].Start);
	WritePrivateProfileString("Input", "P2C.Start", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[6].A);
	WritePrivateProfileString("Input", "P2C.A", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[6].B);
	WritePrivateProfileString("Input", "P2C.B", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[6].C);
	WritePrivateProfileString("Input", "P2C.C", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[6].Mode);
	WritePrivateProfileString("Input", "P2C.Mode", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[6].X);
	WritePrivateProfileString("Input", "P2C.X", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[6].Y);
	WritePrivateProfileString("Input", "P2C.Y", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[6].Z);
	WritePrivateProfileString("Input", "P2C.Z", Str_Tmp, Conf_File);

	wsprintf(Str_Tmp, "%d", Controller_2D_Type & 0x03);
	WritePrivateProfileString("Input", "P2D.Type", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[7].Up);
	WritePrivateProfileString("Input", "P2D.Up", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[7].Down);
	WritePrivateProfileString("Input", "P2D.Down", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[7].Left);
	WritePrivateProfileString("Input", "P2D.Left", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[7].Right);
	WritePrivateProfileString("Input", "P2D.Right", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[7].Start);
	WritePrivateProfileString("Input", "P2D.Start", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[7].A);
	WritePrivateProfileString("Input", "P2D.A", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[7].B);
	WritePrivateProfileString("Input", "P2D.B", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[7].C);
	WritePrivateProfileString("Input", "P2D.C", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[7].Mode);
	WritePrivateProfileString("Input", "P2D.Mode", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[7].X);
	WritePrivateProfileString("Input", "P2D.X", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[7].Y);
	WritePrivateProfileString("Input", "P2D.Y", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", Keys_Def[7].Z);
	WritePrivateProfileString("Input", "P2D.Z", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", QuickLoadKey);
	WritePrivateProfileString("Input", "QuickLoadKey", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", QuickSaveKey);
	WritePrivateProfileString("Input", "QuickSaveKey", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", QuickPauseKey);
	WritePrivateProfileString("Input", "PauseKey", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", SlowDownKey);
	WritePrivateProfileString("Input", "ToggleSlowKey", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", SkipKey);
	WritePrivateProfileString("Input", "SkipFrameKey", Str_Tmp, Conf_File);
	wsprintf(Str_Tmp, "%d", LeftRightEnabled);//Modif
	WritePrivateProfileString("Input", "LeftRightEnabled", Str_Tmp, Conf_File);//Modif
	wsprintf(Str_Tmp, "%d", NumLoadEnabled);//Modif N.
	WritePrivateProfileString("Input", "NumLoadEnabled", Str_Tmp, Conf_File);//Modif N.
	wsprintf(Str_Tmp, "%d", AutoFireKey);//Modif N.
	WritePrivateProfileString("Input", "AutoFireKey", Str_Tmp, Conf_File);//Modif N.
	wsprintf(Str_Tmp, "%d", AutoHoldKey);//Modif N.
	WritePrivateProfileString("Input", "AutoHoldKey", Str_Tmp, Conf_File);//Modif N.
	wsprintf(Str_Tmp, "%d", AutoClearKey);//Modif N.
	WritePrivateProfileString("Input", "AutoClearKey", Str_Tmp, Conf_File);//Modif N.
	wsprintf(Str_Tmp, "%d", StateSelectCfg );//Modif U.
	WritePrivateProfileString("Input", "StateSelectType", Str_Tmp, Conf_File);//Modif N.

	WritePrivateProfileString("Splice","SpliceMovie",SpliceMovie,Conf_File);
	sprintf(Str_Tmp,"%d",SpliceFrame);
	WritePrivateProfileString("Splice","SpliceFrame",Str_Tmp,Conf_File);
	WritePrivateProfileString("Splice","TempFile",TempName,Conf_File);

	return 1;
}


int Save_As_Config(HWND hWnd)
{
	char Name[2048];
	OPENFILENAME ofn;

	SetCurrentDirectory(Gens_Path);

	strcpy(&Name[0], "Gens.cfg");
	memset(&ofn, 0, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = ghInstance;
	ofn.lpstrFile = Name;
	ofn.nMaxFile = 2047;
	ofn.lpstrFilter = "Config Files\0*.cfg\0All Files\0*.*\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrInitialDir = Gens_Path;
	ofn.lpstrDefExt = "cfg";
	ofn.Flags = 0;

	if(GetSaveFileName(&ofn))
	{
		Save_Config(Name);
		strcpy(Str_Tmp, "config saved in ");
		strcat(Str_Tmp, Name);
		Put_Info(Str_Tmp, 2000);
		return 1;
	}
	else return 0;
}


int Load_Config(char *File_Name, void *Game_Active)
{
	int new_val;
	char Conf_File[1024];

	SetCurrentDirectory(Gens_Path);
	strcpy(Conf_File, File_Name);

	CRam_Flag = 1;

	GetPrivateProfileString("General", "Rom path", ".\\", &Rom_Dir[0], 1024, Conf_File);
	GetPrivateProfileString("General", "Save path", Rom_Dir, &State_Dir[0], 1024, Conf_File);
	GetPrivateProfileString("General", "SRAM path", Rom_Dir, &SRAM_Dir[0], 1024, Conf_File);
	GetPrivateProfileString("General", "BRAM path", Rom_Dir, &BRAM_Dir[0], 1024, Conf_File);
	GetPrivateProfileString("General", "Dump path", Rom_Dir, &Dump_Dir[0], 1024, Conf_File);
	GetPrivateProfileString("General", "Dump GYM path", Rom_Dir, &Dump_GYM_Dir[0], 1024, Conf_File);
	GetPrivateProfileString("General", "Screen Shot path", Rom_Dir, &ScrShot_Dir[0], 1024, Conf_File);
	GetPrivateProfileString("General", "Patch path", Rom_Dir, &Patch_Dir[0], 1024, Conf_File);
	GetPrivateProfileString("General", "IPS Patch path", Rom_Dir, &IPS_Dir[0], 1024, Conf_File);
	GetPrivateProfileString("General", "Movie path", Rom_Dir, &Movie_Dir[0], 1024, Conf_File);

	GetPrivateProfileString("General", "Genesis Bios", Rom_Dir, &Genesis_Bios[0], 1024, Conf_File);

	GetPrivateProfileString("General", "USA CD Bios", Rom_Dir, &US_CD_Bios[0], 1024, Conf_File);
	GetPrivateProfileString("General", "EUROPE CD Bios", Rom_Dir, &EU_CD_Bios[0], 1024, Conf_File);
	GetPrivateProfileString("General", "JAPAN CD Bios", Rom_Dir, &JA_CD_Bios[0], 1024, Conf_File);

	GetPrivateProfileString("General", "32X 68000 Bios", Rom_Dir, &_32X_Genesis_Bios[0], 1024, Conf_File);
	GetPrivateProfileString("General", "32X Master SH2 Bios", Rom_Dir, &_32X_Master_Bios[0], 1024, Conf_File);
	GetPrivateProfileString("General", "32X Slave SH2 Bios", Rom_Dir, &_32X_Slave_Bios[0], 1024, Conf_File);

	GetPrivateProfileString("General", "Rom 1", "", &Recent_Rom[0][0], 1024, Conf_File);
	GetPrivateProfileString("General", "Rom 2", "", &Recent_Rom[1][0], 1024, Conf_File);
	GetPrivateProfileString("General", "Rom 3", "", &Recent_Rom[2][0], 1024, Conf_File);
	GetPrivateProfileString("General", "Rom 4", "", &Recent_Rom[3][0], 1024, Conf_File);
	GetPrivateProfileString("General", "Rom 5", "", &Recent_Rom[4][0], 1024, Conf_File);
	GetPrivateProfileString("General", "Rom 6", "", &Recent_Rom[5][0], 1024, Conf_File);
	GetPrivateProfileString("General", "Rom 7", "", &Recent_Rom[6][0], 1024, Conf_File);
	GetPrivateProfileString("General", "Rom 8", "", &Recent_Rom[7][0], 1024, Conf_File);
	GetPrivateProfileString("General", "Rom 9", "", &Recent_Rom[8][0], 1024, Conf_File);
	GetPrivateProfileString("General", "Rom 10", "", &Recent_Rom[9][0], 1024, Conf_File);
	GetPrivateProfileString("General", "Rom 11", "", &Recent_Rom[10][0], 1024, Conf_File);
	GetPrivateProfileString("General", "Rom 12", "", &Recent_Rom[11][0], 1024, Conf_File);
	GetPrivateProfileString("General", "Rom 13", "", &Recent_Rom[12][0], 1024, Conf_File);
	GetPrivateProfileString("General", "Rom 14", "", &Recent_Rom[13][0], 1024, Conf_File);
	GetPrivateProfileString("General", "Rom 15", "", &Recent_Rom[14][0], 1024, Conf_File);

	File_Type_Index = GetPrivateProfileInt("General", "File type index", 1, Conf_File);

	Current_State = GetPrivateProfileInt("General", "State Number", 0, Conf_File);
	Language = GetPrivateProfileInt("General", "Language", 0, Conf_File);
	Window_Pos.x = GetPrivateProfileInt("General", "Window X", 0, Conf_File);
	Window_Pos.y = GetPrivateProfileInt("General", "Window Y", 0, Conf_File);
	Intro_Style = GetPrivateProfileInt("General", "Intro Style", 0, Conf_File); //Modif N. - default to not using the "crazy" intro effect, which used to guzzle more CPU than the actual emulation
	Effect_Color = Intro_Style ? GetPrivateProfileInt("General", "Free Mode Color", 7, Conf_File) : 0; //Modif N. - no intro style = no intro style color
	Sleep_Time = GetPrivateProfileInt("General", "Allow Idle", 5, Conf_File); //Modif N. - CPU hogging now off by default
	Gens_Priority = GetPrivateProfileInt("General", "Priority", 1, Conf_File);
	Def_Read_Only = (bool) (GetPrivateProfileInt("General", "Movie Default Read Only", 1, Conf_File) > 0); //Upth-Add - Load the flag from config
	AutoCloseMovie = (bool) (GetPrivateProfileInt("General", "Movie Auto Close", 1, Conf_File) > 0); //Upth-Add - Load the flag from config
	UseMovieStates = (bool) (GetPrivateProfileInt("General", "Movie Based State Names", 1, Conf_File) > 0); //Upth-Add - Load the flag from config
	SlowDownSpeed = GetPrivateProfileInt("General", "Slow Down Speed", 1, Conf_File); //Upth-Add - Load the slowdown speed from config
	DelayFactor = GetPrivateProfileInt("General", "Frame Advance Delay Factor", 5, Conf_File); //Upth-Add - Frame advance speed configurable

	if (GetPrivateProfileInt("Graphics", "Force 555", 0, Conf_File)) Mode_555 = 3;
	else if (GetPrivateProfileInt("Graphics", "Force 565", 0, Conf_File)) Mode_555 = 2;
	else Mode_555 = 0;

	RMax_Level = GetPrivateProfileInt("Graphics", "Red Max", 255, Conf_File);
	GMax_Level = GetPrivateProfileInt("Graphics", "Green Max", 255, Conf_File);
	BMax_Level = GetPrivateProfileInt("Graphics", "Blue Max", 255, Conf_File);
	Contrast_Level = GetPrivateProfileInt("Graphics", "Contrast", 100, Conf_File);
	Brightness_Level = GetPrivateProfileInt("Graphics", "Brightness", 100, Conf_File);
	Greyscale = GetPrivateProfileInt("Graphics", "Greyscale", 0, Conf_File);
	Invert_Color = GetPrivateProfileInt("Graphics", "Invert", 0, Conf_File);

	Recalculate_Palettes();

	FS_VSync = GetPrivateProfileInt("Graphics", "Full Screen VSync", 0, Conf_File);
	Res_X = GetPrivateProfileInt("Graphics", "Full Screen X Resolution", 0, Conf_File); //Upth-Add - Load the fullscreen
	Res_Y = GetPrivateProfileInt("Graphics", "Full Screen Y Resolution", 0, Conf_File); //Upth-Add - resolution from config
	FS_No_Res_Change = (bool) (GetPrivateProfileInt("Graphics", "Full Screen No Res Change", 0, Conf_File) > 0); //Upth-Add - and the no_res_change flag
	W_VSync = GetPrivateProfileInt("Graphics", "Windows VSync", 0, Conf_File);
	Full_Screen = GetPrivateProfileInt("Graphics", "Full Screen", 0, Conf_File);
	Render_W = GetPrivateProfileInt("Graphics", "Render Windowed", 0, Conf_File);
	Render_FS = GetPrivateProfileInt("Graphics", "Render Fullscreen", 1, Conf_File);
	if (Res_X < 320) Res_X = 320; //Upth-Add - Make sure our resolution
	if (Res_Y < 240) Res_Y = 240; //Upth-Add - is at least 320x240

	Set_Render(HWnd, Full_Screen, -1, 1);

	Stretch = GetPrivateProfileInt("Graphics", "Stretch", 0, Conf_File);
	Blit_Soft = GetPrivateProfileInt("Graphics", "Software Blit", 0, Conf_File);
	Sprite_Over = GetPrivateProfileInt("Graphics", "Sprite limit", 1, Conf_File);
	Frame_Skip = GetPrivateProfileInt("Graphics", "Frame skip", -1, Conf_File);
	CleanAvi = GetPrivateProfileInt("Graphics", "Clean Avi", 1, Conf_File);

	Sound_Rate = GetPrivateProfileInt("Sound", "Rate", 44100, Conf_File);
	Sound_Stereo = GetPrivateProfileInt("Sound", "Stereo", 1, Conf_File);
	Sound_Soften = GetPrivateProfileInt("Sound", "SoundSoftenFilter", 1, Conf_File); // Modif N.

	if (GetPrivateProfileInt("Sound", "Z80 State", 1, Conf_File)) Z80_State |= 1;
	else Z80_State &= ~1;

	new_val = GetPrivateProfileInt("Sound", "State", 1, Conf_File);
	if (new_val != Sound_Enable)
	{
		if (Change_Sound(HWnd))
		{
			YM2612_Enable = GetPrivateProfileInt("Sound", "YM2612 State", 1, Conf_File);
			PSG_Enable = GetPrivateProfileInt("Sound", "PSG State", 1, Conf_File);
			DAC_Enable = GetPrivateProfileInt("Sound", "DAC State", 1, Conf_File);
			PCM_Enable = GetPrivateProfileInt("Sound", "PCM State", 1, Conf_File);
			PWM_Enable = GetPrivateProfileInt("Sound", "PWM State", 1, Conf_File);
			CDDA_Enable = GetPrivateProfileInt("Sound", "CDDA State", 1, Conf_File);
		}
	}
	else
	{
		YM2612_Enable = GetPrivateProfileInt("Sound", "YM2612 State", 1, Conf_File);
		PSG_Enable = GetPrivateProfileInt("Sound", "PSG State", 1, Conf_File);
		DAC_Enable = GetPrivateProfileInt("Sound", "DAC State", 1, Conf_File);
		PCM_Enable = GetPrivateProfileInt("Sound", "PCM State", 1, Conf_File);
		PWM_Enable = GetPrivateProfileInt("Sound", "PWM State", 1, Conf_File);
		CDDA_Enable = GetPrivateProfileInt("Sound", "CDDA State", 1, Conf_File);
	}

	YM2612_Improv = GetPrivateProfileInt("Sound", "YM2612 Improvement", 1, Conf_File); // Modif N
	DAC_Improv = GetPrivateProfileInt("Sound", "DAC Improvement", 1, Conf_File); // Modif N
	PSG_Improv = GetPrivateProfileInt("Sound", "PSG Improvement", 1, Conf_File); // Modif N
	MastVol = (GetPrivateProfileInt("Sound", "Master Volume", 128, Conf_File) & 0x1FF);
	YM2612Vol = (GetPrivateProfileInt("Sound", "YM2612 Volume", 255, Conf_File) & 0x1FF);
	DACVol = (GetPrivateProfileInt("Sound", "DAC Volume", 255, Conf_File) & 0x1FF);
	PSGVol = (GetPrivateProfileInt("Sound", "PSG Volume", 255, Conf_File) & 0x1FF);
	PCMVol = (GetPrivateProfileInt("Sound", "PCM Volume", 255, Conf_File) & 0x1FF);
	CDDAVol = (GetPrivateProfileInt("Sound", "CDDA Volume", 255, Conf_File) & 0x1FF);
	PWMVol = (GetPrivateProfileInt("Sound", "PWM Volume", 255, Conf_File) & 0x1FF);


	Country = GetPrivateProfileInt("CPU", "Country", -1, Conf_File);
	Country_Order[0] = GetPrivateProfileInt("CPU", "Prefered Country 1", 0, Conf_File);
	Country_Order[1] = GetPrivateProfileInt("CPU", "Prefered Country 2", 1, Conf_File);
	Country_Order[2] = GetPrivateProfileInt("CPU", "Prefered Country 3", 2, Conf_File);

	SegaCD_Accurate = GetPrivateProfileInt("CPU", "Perfect synchro between main and sub CPU (Sega CD)", 0, Conf_File);

	MSH2_Speed = GetPrivateProfileInt("CPU", "Main SH2 Speed", 100, Conf_File);
	SSH2_Speed = GetPrivateProfileInt("CPU", "Slave SH2 Speed", 100, Conf_File);

	if (MSH2_Speed < 0) MSH2_Speed = 0;
	if (SSH2_Speed < 0) SSH2_Speed = 0;

	Check_Country_Order();

	Fast_Blur = GetPrivateProfileInt("Options", "Fast Blur", 0, Conf_File);
	Show_FPS = GetPrivateProfileInt("Options", "FPS", 0, Conf_File);
	FPS_Style = GetPrivateProfileInt("Options", "FPS Style", 0, Conf_File);
	Show_Message = GetPrivateProfileInt("Options", "Message", 1, Conf_File);
	Message_Style = GetPrivateProfileInt("Options", "Message Style", 0, Conf_File);
	Show_LED = GetPrivateProfileInt("Options", "LED", 1, Conf_File);
	Auto_Fix_CS = GetPrivateProfileInt("Options", "Auto Fix Checksum", 0, Conf_File);
	Auto_Pause = GetPrivateProfileInt("Options", "Auto Pause", 0, Conf_File);
	CUR_DEV = GetPrivateProfileInt("Options", "CD Drive", 0, Conf_File);
	BRAM_Ex_Size = GetPrivateProfileInt("Options", "Ram Cart Size", 3, Conf_File);

	if (BRAM_Ex_Size == -1)
	{
		BRAM_Ex_State &= 1;
		BRAM_Ex_Size = 0;
	}
	else BRAM_Ex_State |= 0x100;

	GetPrivateProfileString("Options", "GCOffline path", "GCOffline.chm", CGOffline_Path, 1024, Conf_File);
	GetPrivateProfileString("Options", "Gens manual path", "manual.exe", Manual_Path, 1024, Conf_File);
    Disable_Blue_Screen = GetPrivateProfileInt("Options", "Disable Blue Screen", 0, Conf_File); //Modif
	FrameCounterEnabled = GetPrivateProfileInt("Options", "FrameCounterEnabled", 1, Conf_File); //Modif N
	FrameCounterFrames = GetPrivateProfileInt("Options", "FrameCounterFrames", 1, Conf_File); // Modif N
	FrameCounterPosition = GetPrivateProfileInt("Options", "FrameCounterPosition", FRAME_COUNTER_TOP_LEFT, Conf_File);
	LagCounterEnabled = GetPrivateProfileInt("Options", "LagCounterEnabled", 1, Conf_File); //Modif N
	LagCounterFrames = GetPrivateProfileInt("Options", "LagCounterFrames", 1, Conf_File); // Modif N
    ShowInputEnabled = GetPrivateProfileInt("Options", "ShowInputEnabled", 1, Conf_File); //Modif N
	AutoBackupEnabled = GetPrivateProfileInt("Options", "AutoBackupEnabled", 0, Conf_File);
	
	Controller_1_Type = GetPrivateProfileInt("Input", "P1.Type", 1, Conf_File);
	Keys_Def[0].Up = GetPrivateProfileInt("Input", "P1.Up", DIK_UP, Conf_File);
	Keys_Def[0].Down = GetPrivateProfileInt("Input", "P1.Down", DIK_DOWN, Conf_File);
	Keys_Def[0].Left = GetPrivateProfileInt("Input", "P1.Left", DIK_LEFT, Conf_File);
	Keys_Def[0].Right = GetPrivateProfileInt("Input", "P1.Right", DIK_RIGHT, Conf_File);
	Keys_Def[0].Start = GetPrivateProfileInt("Input", "P1.Start", DIK_RETURN, Conf_File);
	Keys_Def[0].A = GetPrivateProfileInt("Input", "P1.A", DIK_A, Conf_File);
	Keys_Def[0].B = GetPrivateProfileInt("Input", "P1.B", DIK_S, Conf_File);
	Keys_Def[0].C = GetPrivateProfileInt("Input", "P1.C", DIK_D, Conf_File);
	Keys_Def[0].Mode = GetPrivateProfileInt("Input", "P1.Mode", DIK_RSHIFT, Conf_File);
	Keys_Def[0].X = GetPrivateProfileInt("Input", "P1.X", DIK_Z, Conf_File);
	Keys_Def[0].Y = GetPrivateProfileInt("Input", "P1.Y", DIK_X, Conf_File);
	Keys_Def[0].Z = GetPrivateProfileInt("Input", "P1.Z", DIK_C, Conf_File);

	Controller_1B_Type = GetPrivateProfileInt("Input", "P1B.Type", 0, Conf_File);
	Keys_Def[2].Up = GetPrivateProfileInt("Input", "P1B.Up", 0, Conf_File);
	Keys_Def[2].Down = GetPrivateProfileInt("Input", "P1B.Down", 0, Conf_File);
	Keys_Def[2].Left = GetPrivateProfileInt("Input", "P1B.Left", 0, Conf_File);
	Keys_Def[2].Right = GetPrivateProfileInt("Input", "P1B.Right", 0, Conf_File);
	Keys_Def[2].Start = GetPrivateProfileInt("Input", "P1B.Start", 0, Conf_File);
	Keys_Def[2].A = GetPrivateProfileInt("Input", "P1B.A", 0, Conf_File);
	Keys_Def[2].B = GetPrivateProfileInt("Input", "P1B.B", 0, Conf_File);
	Keys_Def[2].C = GetPrivateProfileInt("Input", "P1B.C", 0, Conf_File);
	Keys_Def[2].Mode = GetPrivateProfileInt("Input", "P1B.Mode", 0, Conf_File);
	Keys_Def[2].X = GetPrivateProfileInt("Input", "P1B.X", 0, Conf_File);
	Keys_Def[2].Y = GetPrivateProfileInt("Input", "P1B.Y", 0, Conf_File);
	Keys_Def[2].Z = GetPrivateProfileInt("Input", "P1B.Z", 0, Conf_File);

	Controller_1C_Type = GetPrivateProfileInt("Input", "P1C.Type", 0, Conf_File);
	Keys_Def[3].Up = GetPrivateProfileInt("Input", "P1C.Up", 0, Conf_File);
	Keys_Def[3].Down = GetPrivateProfileInt("Input", "P1C.Down", 0, Conf_File);
	Keys_Def[3].Left = GetPrivateProfileInt("Input", "P1C.Left", 0, Conf_File);
	Keys_Def[3].Right = GetPrivateProfileInt("Input", "P1C.Right", 0, Conf_File);
	Keys_Def[3].Start = GetPrivateProfileInt("Input", "P1C.Start", 0, Conf_File);
	Keys_Def[3].A = GetPrivateProfileInt("Input", "P1C.A", 0, Conf_File);
	Keys_Def[3].B = GetPrivateProfileInt("Input", "P1C.B", 0, Conf_File);
	Keys_Def[3].C = GetPrivateProfileInt("Input", "P1C.C", 0, Conf_File);
	Keys_Def[3].Mode = GetPrivateProfileInt("Input", "P1C.Mode", 0, Conf_File);
	Keys_Def[3].X = GetPrivateProfileInt("Input", "P1C.X", 0, Conf_File);
	Keys_Def[3].Y = GetPrivateProfileInt("Input", "P1C.Y", 0, Conf_File);
	Keys_Def[3].Z = GetPrivateProfileInt("Input", "P1C.Z", 0, Conf_File);

	Controller_1D_Type = GetPrivateProfileInt("Input", "P1D.Type", 0, Conf_File);
	Keys_Def[4].Up = GetPrivateProfileInt("Input", "P1D.Up", 0, Conf_File);
	Keys_Def[4].Down = GetPrivateProfileInt("Input", "P1D.Down", 0, Conf_File);
	Keys_Def[4].Left = GetPrivateProfileInt("Input", "P1D.Left", 0, Conf_File);
	Keys_Def[4].Right = GetPrivateProfileInt("Input", "P1D.Right", 0, Conf_File);
	Keys_Def[4].Start = GetPrivateProfileInt("Input", "P1D.Start", 0, Conf_File);
	Keys_Def[4].A = GetPrivateProfileInt("Input", "P1D.A", 0, Conf_File);
	Keys_Def[4].B = GetPrivateProfileInt("Input", "P1D.B", 0, Conf_File);
	Keys_Def[4].C = GetPrivateProfileInt("Input", "P1D.C", 0, Conf_File);
	Keys_Def[4].Mode = GetPrivateProfileInt("Input", "P1D.Mode", 0, Conf_File);
	Keys_Def[4].X = GetPrivateProfileInt("Input", "P1D.X", 0, Conf_File);
	Keys_Def[4].Y = GetPrivateProfileInt("Input", "P1D.Y", 0, Conf_File);
	Keys_Def[4].Z = GetPrivateProfileInt("Input", "P1D.Z", 0, Conf_File);

	Controller_2_Type = GetPrivateProfileInt("Input", "P2.Type", 1, Conf_File);
	Keys_Def[1].Up = GetPrivateProfileInt("Input", "P2.Up", DIK_Y, Conf_File);
	Keys_Def[1].Down = GetPrivateProfileInt("Input", "P2.Down", DIK_H, Conf_File);
	Keys_Def[1].Left = GetPrivateProfileInt("Input", "P2.Left", DIK_G, Conf_File);
	Keys_Def[1].Right = GetPrivateProfileInt("Input", "P2.Right", DIK_J, Conf_File);
	Keys_Def[1].Start = GetPrivateProfileInt("Input", "P2.Start", DIK_U, Conf_File);
	Keys_Def[1].A = GetPrivateProfileInt("Input", "P2.A", DIK_K, Conf_File);
	Keys_Def[1].B = GetPrivateProfileInt("Input", "P2.B", DIK_L, Conf_File);
	Keys_Def[1].C = GetPrivateProfileInt("Input", "P2.C", DIK_M, Conf_File);
	Keys_Def[1].Mode = GetPrivateProfileInt("Input", "P2.Mode", DIK_T, Conf_File);
	Keys_Def[1].X = GetPrivateProfileInt("Input", "P2.X", DIK_I, Conf_File);
	Keys_Def[1].Y = GetPrivateProfileInt("Input", "P2.Y", DIK_O, Conf_File);
	Keys_Def[1].Z = GetPrivateProfileInt("Input", "P2.Z", DIK_P, Conf_File);

	Controller_2B_Type = GetPrivateProfileInt("Input", "P2B.Type", 0, Conf_File);
	Keys_Def[5].Up = GetPrivateProfileInt("Input", "P2B.Up", 0, Conf_File);
	Keys_Def[5].Down = GetPrivateProfileInt("Input", "P2B.Down", 0, Conf_File);
	Keys_Def[5].Left = GetPrivateProfileInt("Input", "P2B.Left", 0, Conf_File);
	Keys_Def[5].Right = GetPrivateProfileInt("Input", "P2B.Right", 0, Conf_File);
	Keys_Def[5].Start = GetPrivateProfileInt("Input", "P2B.Start", 0, Conf_File);
	Keys_Def[5].A = GetPrivateProfileInt("Input", "P2B.A", 0, Conf_File);
	Keys_Def[5].B = GetPrivateProfileInt("Input", "P2B.B", 0, Conf_File);
	Keys_Def[5].C = GetPrivateProfileInt("Input", "P2B.C", 0, Conf_File);
	Keys_Def[5].Mode = GetPrivateProfileInt("Input", "P2B.Mode", 0, Conf_File);
	Keys_Def[5].X = GetPrivateProfileInt("Input", "P2B.X", 0, Conf_File);
	Keys_Def[5].Y = GetPrivateProfileInt("Input", "P2B.Y", 0, Conf_File);
	Keys_Def[5].Z = GetPrivateProfileInt("Input", "P2B.Z", 0, Conf_File);

	Controller_2C_Type = GetPrivateProfileInt("Input", "P2C.Type", 0, Conf_File);
	Keys_Def[6].Up = GetPrivateProfileInt("Input", "P2C.Up", 0, Conf_File);
	Keys_Def[6].Down = GetPrivateProfileInt("Input", "P2C.Down", 0, Conf_File);
	Keys_Def[6].Left = GetPrivateProfileInt("Input", "P2C.Left", 0, Conf_File);
	Keys_Def[6].Right = GetPrivateProfileInt("Input", "P2C.Right", 0, Conf_File);
	Keys_Def[6].Start = GetPrivateProfileInt("Input", "P2C.Start", 0, Conf_File);
	Keys_Def[6].A = GetPrivateProfileInt("Input", "P2C.A", 0, Conf_File);
	Keys_Def[6].B = GetPrivateProfileInt("Input", "P2C.B", 0, Conf_File);
	Keys_Def[6].C = GetPrivateProfileInt("Input", "P2C.C", 0, Conf_File);
	Keys_Def[6].Mode = GetPrivateProfileInt("Input", "P2C.Mode", 0, Conf_File);
	Keys_Def[6].X = GetPrivateProfileInt("Input", "P2C.X", 0, Conf_File);
	Keys_Def[6].Y = GetPrivateProfileInt("Input", "P2C.Y", 0, Conf_File);
	Keys_Def[6].Z = GetPrivateProfileInt("Input", "P2C.Z", 0, Conf_File);

	Controller_2D_Type = GetPrivateProfileInt("Input", "P2D.Type", 0, Conf_File);
	Keys_Def[7].Up = GetPrivateProfileInt("Input", "P2D.Up", 0, Conf_File);
	Keys_Def[7].Down = GetPrivateProfileInt("Input", "P2D.Down", 0, Conf_File);
	Keys_Def[7].Left = GetPrivateProfileInt("Input", "P2D.Left", 0, Conf_File);
	Keys_Def[7].Right = GetPrivateProfileInt("Input", "P2D.Right", 0, Conf_File);
	Keys_Def[7].Start = GetPrivateProfileInt("Input", "P2D.Start", 0, Conf_File);
	Keys_Def[7].A = GetPrivateProfileInt("Input", "P2D.A", 0, Conf_File);
	Keys_Def[7].B = GetPrivateProfileInt("Input", "P2D.B", 0, Conf_File);
	Keys_Def[7].C = GetPrivateProfileInt("Input", "P2D.C", 0, Conf_File);
	Keys_Def[7].Mode = GetPrivateProfileInt("Input", "P2D.Mode", 0, Conf_File);
	Keys_Def[7].X = GetPrivateProfileInt("Input", "P2D.X", 0, Conf_File);
	Keys_Def[7].Y = GetPrivateProfileInt("Input", "P2D.Y", 0, Conf_File);
	Keys_Def[7].Z = GetPrivateProfileInt("Input", "P2D.Z", 0, Conf_File);
	QuickLoadKey = GetPrivateProfileInt("Input", "QuickLoadKey", 0, Conf_File);
	QuickSaveKey = GetPrivateProfileInt("Input", "QuickSaveKey", 0, Conf_File);
	AutoFireKey = GetPrivateProfileInt("Input", "AutoFireKey", 0, Conf_File); //Modif N
	AutoHoldKey = GetPrivateProfileInt("Input", "AutoHoldKey", 0, Conf_File); //Modif N
	AutoClearKey = GetPrivateProfileInt("Input", "AutoClearKey", 0, Conf_File); //Modif N
	QuickPauseKey = GetPrivateProfileInt("Input", "PauseKey", 0, Conf_File);
	SlowDownKey = GetPrivateProfileInt("Input", "ToggleSlowKey", 0, Conf_File);
	SkipKey = GetPrivateProfileInt("Input", "SkipFrameKey", 0, Conf_File);
	LeftRightEnabled = GetPrivateProfileInt("Input", "LeftRightEnabled", 0, Conf_File);
	NumLoadEnabled = GetPrivateProfileInt("Input", "NumLoadEnabled", 1, Conf_File); //Modif N
	StateSelectCfg = GetPrivateProfileInt("Input", "StateSelectType", 0, Conf_File); //Modif N

	GetPrivateProfileString("Splice","SpliceMovie","",SpliceMovie,1024,Conf_File);
	SpliceFrame = GetPrivateProfileInt("Splice","SpliceFrame",0,Conf_File);
	GetPrivateProfileString("Splice","TempFile","",Str_Tmp,1024,Conf_File);
	if (SpliceFrame)
	{
		TempName = (char *)malloc(strlen(Str_Tmp)+2);
		strcpy(TempName,Str_Tmp);
		sprintf(Str_Tmp,"Incomplete splice session detected for %s.\n\
						 Do you wish to continue this session?\n\
						 (Warning: If you do not restore the session, it will be permanently discarded.)",SpliceMovie);
		int response = MessageBox(NULL,Str_Tmp,"NOTICE",MB_YESNO | MB_ICONQUESTION);
		while ((response != IDYES) && (response != IDNO))
			response = MessageBox(NULL,Str_Tmp,"NOTICE",MB_YESNO | MB_ICONQUESTION);
		if (response == IDNO)
		{
			if (MessageBox(NULL,"Restore gmv from backup?","PROMPT",MB_YESNO | MB_ICONQUESTION) == IDYES)
			{
				strcpy(Str_Tmp,SpliceMovie);
				Str_Tmp[strlen(Str_Tmp)-3] = 0;
				strcat(Str_Tmp,"spl.gmv");
				MainMovie.File = fopen(Str_Tmp,"rb");
				if (!MainMovie.File)
					MessageBox(NULL,"Error opening movie backup.","ERROR",MB_OK | MB_ICONWARNING);
				else 
				{
					strcpy(MainMovie.FileName,SpliceMovie);
					BackupMovieFile(&MainMovie);
					strcpy(MainMovie.FileName,"");
					fclose(MainMovie.File);
					remove(Str_Tmp);
				}
			}
			remove(TempName);
			free(TempName);
			TempName = NULL;
			SpliceFrame = 0;
			strcpy(SpliceMovie,"");
		}
		else
		{
			sprintf(Str_Tmp,"%s splice session restored",SpliceMovie);
			Put_Info(Str_Tmp,2000);
		}
	}

	Make_IO_Table();
	DestroyMenu(Gens_Menu);

	if (Full_Screen) Build_Main_Menu();
	else SetMenu(HWnd, Build_Main_Menu());		// Update new menu

	return 1;
}


int Load_As_Config(HWND hWnd, void *Game_Active)
{
	char Name[2048];
	OPENFILENAME ofn;

	SetCurrentDirectory(Gens_Path);

	strcpy(&Name[0], "Gens.cfg");
	memset(&ofn, 0, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = ghInstance;
	ofn.lpstrFile = Name;
	ofn.nMaxFile = 2047;
	ofn.lpstrFilter = "Config Files\0*.cfg\0All Files\0*.*\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrInitialDir = Gens_Path;
	ofn.lpstrDefExt = "cfg";
	ofn.Flags = OFN_FILEMUSTEXIST;

	if(GetOpenFileName(&ofn))
	{
		Load_Config(Name, Game_Active);
		strcpy(Str_Tmp, "config loaded from ");
		strcat(Str_Tmp, Name);
		Put_Info(Str_Tmp, 2000);
		return 1;
	}
	else return 0;
}


int Load_SRAM(void)
{
	int bResult;
	HANDLE SRAM_File;
	char Name[2048];
	unsigned long Bytes_Read;

	SetCurrentDirectory(Gens_Path);

	memset(SRAM, 0, 64 * 1024);

	strcpy(Name, SRAM_Dir);
	strcat(Name, Rom_Name);
	strcat(Name, ".srm");
	
	SRAM_File = CreateFile(Name, GENERIC_READ, FILE_SHARE_READ, 
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (SRAM_File == INVALID_HANDLE_VALUE) return 0;

	bResult = ReadFile(SRAM_File, SRAM, 64 * 1024, &Bytes_Read, NULL);

	CloseHandle(SRAM_File);

	strcpy(Str_Tmp, "SRAM loaded from ");
	strcat(Str_Tmp, Name);
	Put_Info(Str_Tmp, 2000);

	return 1;
}
int Load_SRAMFromBuf(char *buf)
{
	strcpy((char *)SRAM,buf);
//	strcpy(Str_Tmp, "SRAM loaded from embedded movie data.");
	Put_Info(Str_Tmp, 2000);

	return 1;
}

int Save_SRAM(void)
{
	HANDLE SRAM_File;
	int bResult, size_to_save, i;
	char Name[2048];
	unsigned long Bytes_Write;

	SetCurrentDirectory(Gens_Path);

	i = (64 * 1024) - 1;
	while ((i >= 0) && (SRAM[i] == 0)) i--;

	if (i < 0) return 0;

	i++;

	size_to_save = 1;
	while (i > size_to_save) size_to_save <<= 1;

	strcpy(Name, SRAM_Dir);
	strcat(Name, Rom_Name);
	strcat(Name, ".srm");
	
	SRAM_File = CreateFile(Name, GENERIC_WRITE, NULL, 
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (SRAM_File == INVALID_HANDLE_VALUE) return 0;
	
	bResult = WriteFile(SRAM_File, SRAM, size_to_save, &Bytes_Write, NULL);
	
	CloseHandle(SRAM_File);

	strcpy(Str_Tmp, "SRAM saved in ");
	strcat(Str_Tmp, Name);
	Put_Info(Str_Tmp, 2000);

	return 1;
}


void S_Format_BRAM(unsigned char *buf)
{
	memset(buf, 0x5F, 11);

	buf[0x0F] = 0x40;

	buf[0x11] = 0x7D;
	buf[0x13] = 0x7D;
	buf[0x15] = 0x7D;
	buf[0x17] = 0x7D;

	sprintf((char *) &buf[0x20], "SEGA CD ROM");
	sprintf((char *) &buf[0x30], "RAM CARTRIDGE");

	buf[0x24] = 0x5F;
	buf[0x27] = 0x5F;

	buf[0x2C] = 0x01;

	buf[0x33] = 0x5F;
	buf[0x3D] = 0x5F;
	buf[0x3E] = 0x5F;
	buf[0x3F] = 0x5F;
}


void Format_Backup_Ram(void)
{
	memset(Ram_Backup, 0, 8 * 1024);

	S_Format_BRAM(&Ram_Backup[0x1FC0]);

	memset(Ram_Backup_Ex, 0, 64 * 1024);
}


int Load_BRAM(void)
{
	HANDLE BRAM_File;
	int bResult;
	char Name[2048];
	unsigned long Bytes_Read;

	Format_Backup_Ram();

	SetCurrentDirectory(Gens_Path);

	strcpy(Name, BRAM_Dir);
	strcat(Name, Rom_Name);
	strcat(Name, ".brm");
	
	BRAM_File = CreateFile(Name, GENERIC_READ, FILE_SHARE_READ, 
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (BRAM_File == INVALID_HANDLE_VALUE) return 0;

	bResult = ReadFile(BRAM_File, Ram_Backup, 8 * 1024, &Bytes_Read, NULL);
	bResult = ReadFile(BRAM_File, Ram_Backup_Ex, (8 << BRAM_Ex_Size) * 1024, &Bytes_Read, NULL);

	CloseHandle(BRAM_File);

	strcpy(Str_Tmp, "BRAM loaded from ");
	strcat(Str_Tmp, Name);
	Put_Info(Str_Tmp, 2000);

	return 1;
}


int Save_BRAM(void)
{
	HANDLE BRAM_File;
	int bResult;
	char Name[2048];
	unsigned long Bytes_Write;

	SetCurrentDirectory(Gens_Path);

	strcpy(Name, BRAM_Dir);
	strcat(Name, Rom_Name);
	strcat(Name, ".brm");
	
	BRAM_File = CreateFile(Name, GENERIC_WRITE, NULL, 
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (BRAM_File == INVALID_HANDLE_VALUE) return 0;
	
	bResult = WriteFile(BRAM_File, Ram_Backup, 8 * 1024, &Bytes_Write, NULL);
	bResult = WriteFile(BRAM_File, Ram_Backup_Ex, (8 << BRAM_Ex_Size) * 1024, &Bytes_Write, NULL);
	
	CloseHandle(BRAM_File);

	strcpy(Str_Tmp, "BRAM saved in ");
	strcat(Str_Tmp, Name);
	Put_Info(Str_Tmp, 2000);

	return 1;
}

