/*****************************************************************************
* | File      	:   EPD_IT8951.c
* | Author      :   Waveshare team
* | Function    :   IT8951 Common driver
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2019-09-17
* | Info        :
* -----------------------------------------------------------------------------
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "EPD_IT8951.h"
#include <time.h>

// Define telegram and burst sizes.
#define TELEGRAM_ROWS 100    // Number of rows per telegram.
#define BURST_SIZE    8190   // Maximum number of 16-bit words per burst.
// Buffer size: 2 bytes for preamble + 2 bytes per word.
#define SPI_BUFFER_SIZE (2 + (BURST_SIZE * 2))


//basic mode definition
UBYTE INIT_Mode = 0;
UBYTE GC16_Mode = 2;
//A2_Mode's value is not fixed, is decide by firmware's LUT 
UBYTE A2_Mode = 6;

/******************************************************************************
function :	Software reset
parameter:
******************************************************************************/
static void EPD_IT8951_Reset(void)
{
    DEV_Digital_Write(EPD_RST_PIN, HIGH);
    DEV_Delay_ms(200);
    DEV_Digital_Write(EPD_RST_PIN, LOW);
    DEV_Delay_ms(10);
    DEV_Digital_Write(EPD_RST_PIN, HIGH);
    DEV_Delay_ms(200);
}


/******************************************************************************
function :	Wait until the busy_pin goes HIGH
parameter:
******************************************************************************/
static void EPD_IT8951_ReadBusy(void)
{
	// Debug("Busy ------\r\n");
    UBYTE Busy_State = DEV_Digital_Read(EPD_BUSY_PIN);
    //0: busy, 1: idle
    while(Busy_State == 0) {
        Busy_State = DEV_Digital_Read(EPD_BUSY_PIN);
    }
	// Debug("Busy Release ------\r\n");
}


/******************************************************************************
function :	write command
parameter:  command
******************************************************************************/
static void EPD_IT8951_WriteCommand(UWORD Command)
{
	//Set Preamble for Write Command
	UWORD Write_Preamble = 0x6000;
	
	EPD_IT8951_ReadBusy();

    DEV_Digital_Write(EPD_CS_PIN, LOW);
	
	DEV_SPI_WriteByte(Write_Preamble>>8);
	DEV_SPI_WriteByte(Write_Preamble);
	
	EPD_IT8951_ReadBusy();	
	
	DEV_SPI_WriteByte(Command>>8);
	DEV_SPI_WriteByte(Command);
	
	DEV_Digital_Write(EPD_CS_PIN, HIGH);
}


/******************************************************************************
function :	write data
parameter:  data
******************************************************************************/
static void EPD_IT8951_WriteData(UWORD Data)
{
    //Set Preamble for Write Command
	UWORD Write_Preamble = 0x0000;

    EPD_IT8951_ReadBusy();

    DEV_Digital_Write(EPD_CS_PIN, LOW);

	DEV_SPI_WriteByte(Write_Preamble>>8);
	DEV_SPI_WriteByte(Write_Preamble);

    EPD_IT8951_ReadBusy();

	DEV_SPI_WriteByte(Data>>8);
	DEV_SPI_WriteByte(Data);

    DEV_Digital_Write(EPD_CS_PIN, HIGH);
}


/******************************************************************************
function :	write multi data
parameter:  data
******************************************************************************/
static void EPD_IT8951_WriteMuitiData(UWORD* Data_Buf, UDOUBLE Length)
{
    //Set Preamble for Write Command
	UWORD Write_Preamble = 0x0000;

    EPD_IT8951_ReadBusy();

    DEV_Digital_Write(EPD_CS_PIN, LOW);

	DEV_SPI_WriteByte(Write_Preamble>>8);
	DEV_SPI_WriteByte(Write_Preamble);

    EPD_IT8951_ReadBusy();

    for(UDOUBLE i = 0; i<Length; i++)
    {
	    DEV_SPI_WriteByte(Data_Buf[i]>>8);
	    DEV_SPI_WriteByte(Data_Buf[i]);
    }
    DEV_Digital_Write(EPD_CS_PIN, HIGH);
}

/******************************************************************************
function :	write multi data burst
parameter:  data
******************************************************************************/

static void EPD_IT8951_WriteMuitiData_Burst(UWORD* Data_Buf, UDOUBLE TotalLength)
{
    UWORD Write_Preamble = 0x0000;
    UDOUBLE remaining = TotalLength;
    UDOUBLE offset = 0;

    while (remaining > 0)
    {
        UDOUBLE burst = (remaining > BURST_SIZE) ? BURST_SIZE : remaining;
        
        // Wait for controller to be ready before starting a burst.
        EPD_IT8951_ReadBusy();
        
        // Begin burst: assert CS and send the preamble.
        DEV_Digital_Write(EPD_CS_PIN, LOW);
        DEV_SPI_WriteByte(Write_Preamble >> 8);
        DEV_SPI_WriteByte(Write_Preamble & 0xFF);
        EPD_IT8951_ReadBusy();
        
        // Send a burst of words.
        for (UDOUBLE i = 0; i < burst; i++)
        {
            UWORD word = Data_Buf[offset + i];
            DEV_SPI_WriteByte(word >> 8);
            DEV_SPI_WriteByte(word & 0xFF);
        }
        
        // End burst: deassert CS.
        DEV_Digital_Write(EPD_CS_PIN, HIGH);
        
        // Update counters.
        offset    += burst;
        remaining -= burst;
        
        // Optional: short delay between bursts.
         DEV_Delay_ms(1);
    }
}



/******************************************************************************
function :	read data
parameter:  data
******************************************************************************/
static UWORD EPD_IT8951_ReadData()
{
    UWORD ReadData;
	UWORD Write_Preamble = 0x1000;
    UWORD Read_Dummy;

    EPD_IT8951_ReadBusy();

    DEV_Digital_Write(EPD_CS_PIN, LOW);

	DEV_SPI_WriteByte(Write_Preamble>>8);
	DEV_SPI_WriteByte(Write_Preamble);

    EPD_IT8951_ReadBusy();

    //dummy
    Read_Dummy = DEV_SPI_ReadByte()<<8;
    Read_Dummy |= DEV_SPI_ReadByte();

    EPD_IT8951_ReadBusy();

    ReadData = DEV_SPI_ReadByte()<<8;
    ReadData |= DEV_SPI_ReadByte();

    DEV_Digital_Write(EPD_CS_PIN, HIGH);

    return ReadData;
}




/******************************************************************************
function :	read multi data
parameter:  data
******************************************************************************/
static void EPD_IT8951_ReadMultiData(UWORD* Data_Buf, UDOUBLE Length)
{
	UWORD Write_Preamble = 0x1000;
    UWORD Read_Dummy;

    EPD_IT8951_ReadBusy();

    DEV_Digital_Write(EPD_CS_PIN, LOW);

	DEV_SPI_WriteByte(Write_Preamble>>8);
	DEV_SPI_WriteByte(Write_Preamble);

    EPD_IT8951_ReadBusy();

    //dummy
    Read_Dummy = DEV_SPI_ReadByte()<<8;
    Read_Dummy |= DEV_SPI_ReadByte();

    EPD_IT8951_ReadBusy();

    for(UDOUBLE i = 0; i<Length; i++)
    {
	    Data_Buf[i] = DEV_SPI_ReadByte()<<8;
	    Data_Buf[i] |= DEV_SPI_ReadByte();
    }

    DEV_Digital_Write(EPD_CS_PIN, HIGH);
}



/******************************************************************************
function:	write multi arg
parameter:	data
description:	some situation like this:
* 1 commander     0    argument
* 1 commander     1    argument
* 1 commander   multi  argument
******************************************************************************/
static void EPD_IT8951_WriteMultiArg(UWORD Arg_Cmd, UWORD* Arg_Buf, UWORD Arg_Num)
{
     //Send Cmd code
     EPD_IT8951_WriteCommand(Arg_Cmd);
     //Send Data
     for(UWORD i=0; i<Arg_Num; i++)
     {
         EPD_IT8951_WriteData(Arg_Buf[i]);
     }
}


/******************************************************************************
function :	Cmd4 ReadReg
parameter:  
******************************************************************************/
static UWORD EPD_IT8951_ReadReg(UWORD Reg_Address)
{
    UWORD Reg_Value;
    EPD_IT8951_WriteCommand(IT8951_TCON_REG_RD);
    EPD_IT8951_WriteData(Reg_Address);
    Reg_Value =  EPD_IT8951_ReadData();
    return Reg_Value;
}



/******************************************************************************
function :	Cmd5 WriteReg
parameter:  
******************************************************************************/
static void EPD_IT8951_WriteReg(UWORD Reg_Address,UWORD Reg_Value)
{
    EPD_IT8951_WriteCommand(IT8951_TCON_REG_WR);
    EPD_IT8951_WriteData(Reg_Address);
    EPD_IT8951_WriteData(Reg_Value);
}



/******************************************************************************
function :	get VCOM
parameter:  
******************************************************************************/
static UWORD EPD_IT8951_GetVCOM(void)
{
    UWORD VCOM;
    EPD_IT8951_WriteCommand(USDEF_I80_CMD_VCOM);
    EPD_IT8951_WriteData(0x0000);
    VCOM =  EPD_IT8951_ReadData();
    return VCOM;
}



/******************************************************************************
function :	set VCOM
parameter:  
******************************************************************************/
static void EPD_IT8951_SetVCOM(UWORD VCOM)
{
    EPD_IT8951_WriteCommand(USDEF_I80_CMD_VCOM);
    EPD_IT8951_WriteData(0x0001);
    EPD_IT8951_WriteData(VCOM);
}



/******************************************************************************
function :	Cmd10 LD_IMG
parameter:  
******************************************************************************/
static void EPD_IT8951_LoadImgStart( IT8951_Load_Img_Info* Load_Img_Info )
{
    UWORD Args;
    Args = (\
        Load_Img_Info->Endian_Type<<8 | \
        Load_Img_Info->Pixel_Format<<4 | \
        Load_Img_Info->Rotate\
    );
    EPD_IT8951_WriteCommand(IT8951_TCON_LD_IMG);
    EPD_IT8951_WriteData(Args);
}


/******************************************************************************
function :	Cmd11 LD_IMG_Area
parameter:  
******************************************************************************/
static void EPD_IT8951_LoadImgAreaStart( IT8951_Load_Img_Info* Load_Img_Info, IT8951_Area_Img_Info* Area_Img_Info )
{
    UWORD Args[5];
    Args[0] = (\
        Load_Img_Info->Endian_Type<<8 | \
        Load_Img_Info->Pixel_Format<<4 | \
        Load_Img_Info->Rotate\
    );
    Args[1] = Area_Img_Info->Area_X;
    Args[2] = Area_Img_Info->Area_Y;
    Args[3] = Area_Img_Info->Area_W;
    Args[4] = Area_Img_Info->Area_H;
    EPD_IT8951_WriteMultiArg(IT8951_TCON_LD_IMG_AREA, Args,5);
}

/******************************************************************************
function :	Cmd12 LD_IMG_End
parameter:  
******************************************************************************/
static void EPD_IT8951_LoadImgEnd(void)
{
    EPD_IT8951_WriteCommand(IT8951_TCON_LD_IMG_END);
}


/******************************************************************************
function :	EPD_IT8951_Get_System_Info
parameter:  
******************************************************************************/
static void EPD_IT8951_GetSystemInfo(void* Buf)
{
    IT8951_Dev_Info* Dev_Info; 

    EPD_IT8951_WriteCommand(USDEF_I80_CMD_GET_DEV_INFO);

    EPD_IT8951_ReadMultiData((UWORD*)Buf, sizeof(IT8951_Dev_Info)/2);

    Dev_Info = (IT8951_Dev_Info*)Buf;
	Debug("Panel(W,H) = (%d,%d)\r\n",Dev_Info->Panel_W, Dev_Info->Panel_H );
	Debug("Memory Address = %X\r\n",Dev_Info->Memory_Addr_L | (Dev_Info->Memory_Addr_H << 16));
	Debug("FW Version = %s\r\n", (UBYTE*)Dev_Info->FW_Version);
	Debug("LUT Version = %s\r\n", (UBYTE*)Dev_Info->LUT_Version);
}


/******************************************************************************
function :	EPD_IT8951_Set_Target_Memory_Addr
parameter:  
******************************************************************************/
static void EPD_IT8951_SetTargetMemoryAddr(UDOUBLE Target_Memory_Addr)
{
	UWORD WordH = (UWORD)((Target_Memory_Addr >> 16) & 0x0000FFFF);
	UWORD WordL = (UWORD)( Target_Memory_Addr & 0x0000FFFF);

    EPD_IT8951_WriteReg(LISAR+2, WordH);
    EPD_IT8951_WriteReg(LISAR  , WordL);
}


/******************************************************************************
function :	EPD_IT8951_WaitForDisplayReady
parameter:  
******************************************************************************/
static void EPD_IT8951_WaitForDisplayReady(void)
{
    //Check IT8951 Register LUTAFSR => NonZero Busy, Zero - Free
    while( EPD_IT8951_ReadReg(LUTAFSR) )
    {
        //wait in idle state
    }
}





/******************************************************************************
function :	EPD_IT8951_HostAreaPackedPixelWrite_1bp
parameter:  
******************************************************************************/
static void EPD_IT8951_HostAreaPackedPixelWrite_1bp(IT8951_Load_Img_Info*Load_Img_Info,IT8951_Area_Img_Info*Area_Img_Info, bool Packed_Write)
{
    UWORD Source_Buffer_Width, Source_Buffer_Height;
    UWORD Source_Buffer_Length;

    UWORD* Source_Buffer = (UWORD*)Load_Img_Info->Source_Buffer_Addr;
    EPD_IT8951_SetTargetMemoryAddr(Load_Img_Info->Target_Memory_Addr);
    EPD_IT8951_LoadImgAreaStart(Load_Img_Info,Area_Img_Info);

    //from byte to word
    //use 8bp to display 1bp, so here, divide by 2, because every byte has full bit.
    Source_Buffer_Width = Area_Img_Info->Area_W/2;
    Source_Buffer_Height = Area_Img_Info->Area_H;
    Source_Buffer_Length = Source_Buffer_Width * Source_Buffer_Height;
    
    if(Packed_Write == true)
    {
        EPD_IT8951_WriteMuitiData(Source_Buffer, Source_Buffer_Length);
    }
    else
    {
        for(UDOUBLE i=0; i<Source_Buffer_Height; i++)
        {
            for(UDOUBLE j=0; j<Source_Buffer_Width; j++)
            {
                EPD_IT8951_WriteData(*Source_Buffer);
                Source_Buffer++;
            }
        }
    }

    EPD_IT8951_LoadImgEnd();
}





/******************************************************************************
function :	EPD_IT8951_HostAreaPackedPixelWrite_2bp
parameter:  
******************************************************************************/
static void EPD_IT8951_HostAreaPackedPixelWrite_2bp(IT8951_Load_Img_Info*Load_Img_Info, IT8951_Area_Img_Info*Area_Img_Info, bool Packed_Write)
{
    UWORD Source_Buffer_Width, Source_Buffer_Height;
    UWORD Source_Buffer_Length;

    UWORD* Source_Buffer = (UWORD*)Load_Img_Info->Source_Buffer_Addr;
    EPD_IT8951_SetTargetMemoryAddr(Load_Img_Info->Target_Memory_Addr);
    EPD_IT8951_LoadImgAreaStart(Load_Img_Info,Area_Img_Info);

    //from byte to word
    Source_Buffer_Width = (Area_Img_Info->Area_W*2/8)/2;
    Source_Buffer_Height = Area_Img_Info->Area_H;
    Source_Buffer_Length = Source_Buffer_Width * Source_Buffer_Height;

    if(Packed_Write == true)
    {
        EPD_IT8951_WriteMuitiData(Source_Buffer, Source_Buffer_Length);
    }
    else
    {
        for(UDOUBLE i=0; i<Source_Buffer_Height; i++)
        {
            for(UDOUBLE j=0; j<Source_Buffer_Width; j++)
            {
                EPD_IT8951_WriteData(*Source_Buffer);
                Source_Buffer++;
            }
        }
    }

    EPD_IT8951_LoadImgEnd();
}





/******************************************************************************
function :	EPD_IT8951_HostAreaPackedPixelWrite_4bp
parameter:  
******************************************************************************/
static void EPD_IT8951_HostAreaPackedPixelWrite_4bp(IT8951_Load_Img_Info*Load_Img_Info, IT8951_Area_Img_Info*Area_Img_Info, bool Packed_Write)
{
    UWORD Source_Buffer_Width, Source_Buffer_Height;
    UWORD Source_Buffer_Length;
	
    UWORD* Source_Buffer = (UWORD*)Load_Img_Info->Source_Buffer_Addr;
    EPD_IT8951_SetTargetMemoryAddr(Load_Img_Info->Target_Memory_Addr);
    EPD_IT8951_LoadImgAreaStart(Load_Img_Info,Area_Img_Info);

    //from byte to word
    Source_Buffer_Width = (Area_Img_Info->Area_W*4/8)/2;    
    Source_Buffer_Height = Area_Img_Info->Area_H;
    Source_Buffer_Length = Source_Buffer_Width * Source_Buffer_Height;    

    if(Packed_Write == true)
    {
        EPD_IT8951_WriteMuitiData(Source_Buffer, Source_Buffer_Length);
    }
    else
    {
        for(UDOUBLE i=0; i<Source_Buffer_Height; i++)
        {
            for(UDOUBLE j=0; j<Source_Buffer_Width; j++)
            {
                EPD_IT8951_WriteData(*Source_Buffer);
                Source_Buffer++;
            }
        }
    }

    EPD_IT8951_LoadImgEnd();
}

static void EPD_IT8951_HostAreaPackedPixelWrite_4bp_PerRow(IT8951_Load_Img_Info* Load_Img_Info,
                                                              IT8951_Area_Img_Info* Area_Img_Info)
{
    // Compute the number of words per row.
    // Original code assumed: (Area_W * 4 / 8) / 2  which is Area_W/4 if no rounding issues.
    // It may be necessary to account for line padding if required by the controller.
    UWORD words_per_row = (Area_Img_Info->Area_W * 4 / 8) / 2;  
    UWORD num_rows = Area_Img_Info->Area_H;
    
    // Pointer to the start of the image data.
    UWORD* Source_Buffer = (UWORD*)Load_Img_Info->Source_Buffer_Addr;
    
    // Loop through each row.
    for (UWORD row = 0; row < num_rows; row++)
    {
        // Prepare a row-specific area info.
        IT8951_Area_Img_Info row_area = *Area_Img_Info;
        row_area.Area_Y = Area_Img_Info->Area_Y + row;
        row_area.Area_H = 1;  // One row at a time.
        
        // Set target memory address if needed.
        EPD_IT8951_SetTargetMemoryAddr(Load_Img_Info->Target_Memory_Addr);
        
        // Start the load image command for this row.
        EPD_IT8951_LoadImgAreaStart(Load_Img_Info, &row_area);
        
        // Write this row’s data in one packed burst.
        EPD_IT8951_WriteMuitiData(&Source_Buffer[row * words_per_row], words_per_row);
        
        // End the image load for this row.
        EPD_IT8951_LoadImgEnd();
        
        // Optional: wait for the controller between rows.
        EPD_IT8951_ReadBusy();
    }
}

// Combined per-telegram & burst-based write for 4bpp images.
static void EPD_IT8951_HostAreaPackedPixelWrite_4bp_Telegram(IT8951_Load_Img_Info* Load_Img_Info,
                                                              IT8951_Area_Img_Info* Area_Img_Info)
{
    // For 4 bits per pixel, each pixel is 0.5 byte.
    // Therefore, for Area_W pixels: (Area_W / 2) bytes total.
    // Each 16-bit word holds 2 bytes: words_per_row = (Area_W / 2) / 2 = Area_W / 4.
    UWORD words_per_row = (Area_Img_Info->Area_W * 4 / 8) / 2;
    UWORD total_rows = Area_Img_Info->Area_H;
    UWORD* Source_Buffer = (UWORD*)Load_Img_Info->Source_Buffer_Addr;
    UWORD current_row = 0;
    
    // Temporary buffer for SPI burst transfers.
    uint8_t spi_tx_buffer[SPI_BUFFER_SIZE];
    
    while (current_row < total_rows)
    {
        // Determine the number of rows to send in this telegram.
        UWORD telegram_rows = TELEGRAM_ROWS;
        if ((total_rows - current_row) < telegram_rows)
        {
            telegram_rows = total_rows - current_row;
        }
        
        // Prepare a telegram-specific area structure.
        IT8951_Area_Img_Info telegram_area = *Area_Img_Info;
        telegram_area.Area_Y = Area_Img_Info->Area_Y + current_row;
        telegram_area.Area_H = telegram_rows;
        
        // Set target memory address and start the load image command.
        EPD_IT8951_SetTargetMemoryAddr(Load_Img_Info->Target_Memory_Addr);
        EPD_IT8951_LoadImgAreaStart(Load_Img_Info, &telegram_area);
        
        // Calculate total number of 16-bit words for this telegram.
        UDOUBLE telegram_words = (UDOUBLE)words_per_row * telegram_rows;
        UDOUBLE remaining = telegram_words;
        UDOUBLE offset = (UDOUBLE)current_row * words_per_row;
        
        // Loop: send data in bursts.
        while (remaining > 0)
        {
            // Determine the burst size.
            UDOUBLE burst = (remaining > BURST_SIZE) ? BURST_SIZE : remaining;
            
            // Wait until the controller is ready.
            EPD_IT8951_ReadBusy();
            
            // Build the SPI transmit buffer:
            // First 2 bytes: write command preamble (0x0000).
            spi_tx_buffer[0] = 0x00;
            spi_tx_buffer[1] = 0x00;
            
            // Next: burst data (each 16-bit word becomes 2 bytes, big-endian).
            for (UDOUBLE i = 0; i < burst; i++)
            {
                UWORD word = Source_Buffer[offset + i];
                spi_tx_buffer[2 + (i * 2)]     = (word >> 8) & 0xFF;
                spi_tx_buffer[2 + (i * 2) + 1] = word & 0xFF;
            }
            
            // Assert chip select, optionally check busy state.
            DEV_Digital_Write(EPD_CS_PIN, LOW);
            EPD_IT8951_ReadBusy();
            
            // Transfer the entire buffer in one bulk SPI call.
            DEV_SPI_WriteBuffer(spi_tx_buffer, 2 + (burst * 2));
            
            // Deassert chip select.
            DEV_Digital_Write(EPD_CS_PIN, HIGH);
            
            offset    += burst;
            remaining -= burst;
        }
        
        // End the load image command for the current telegram.
        EPD_IT8951_LoadImgEnd();
        EPD_IT8951_ReadBusy();
        current_row += telegram_rows;
    }
}

static void EPD_IT8951_HostAreaPackedPixelWrite_4bp_WholeImage(IT8951_Load_Img_Info* Load_Img_Info,
                                                                 IT8951_Area_Img_Info* Area_Img_Info)
{
    // For 4bpp: each pixel uses 0.5 byte.
    // Thus, for Area_W pixels: (Area_W / 2) bytes.
    // Each 16-bit word holds 2 bytes, so:
    //    words_per_row = (Area_W / 2) / 2 = Area_W / 4.
    UWORD words_per_row = (Area_Img_Info->Area_W * 4 / 8) / 2;
    UWORD total_rows = Area_Img_Info->Area_H;
    UWORD* Source_Buffer = (UWORD*)Load_Img_Info->Source_Buffer_Addr;

    // Allocate a temporary buffer for SPI burst transfers.
    uint8_t spi_tx_buffer[SPI_BUFFER_SIZE];

    // Use one telegram for the whole image.
    IT8951_Area_Img_Info whole_area = *Area_Img_Info;
    // whole_area.Area_X and whole_area.Area_Y remain unchanged;
    // whole_area.Area_H equals total_rows.
    
    // Set target memory address and start the load image command.
    EPD_IT8951_SetTargetMemoryAddr(Load_Img_Info->Target_Memory_Addr);
    EPD_IT8951_LoadImgAreaStart(Load_Img_Info, &whole_area);

    // Calculate total number of 16-bit words in the image.
    UDOUBLE total_words = (UDOUBLE)words_per_row * total_rows;
    UDOUBLE remaining = total_words;
    UDOUBLE offset = 0;

    // Send the entire image data in bursts.
    while (remaining > 0)
    {
        // Determine how many words to send in this burst.
        UDOUBLE burst = (remaining > BURST_SIZE) ? BURST_SIZE : remaining;

        // Wait until the controller is ready.
        EPD_IT8951_ReadBusy();

        // Build the SPI transmit buffer:
        // First 2 bytes: write command preamble (0x0000).
        spi_tx_buffer[0] = 0x00;
        spi_tx_buffer[1] = 0x00;

        // Next: burst data (each 16-bit word converted to 2 bytes, big-endian).
        for (UDOUBLE i = 0; i < burst; i++)
        {
            UWORD word = Source_Buffer[offset + i];
            spi_tx_buffer[2 + (i * 2)]     = (word >> 8) & 0xFF;
            spi_tx_buffer[2 + (i * 2) + 1] = word & 0xFF;
        }

        // Assert chip select and (optionally) check busy state.
        DEV_Digital_Write(EPD_CS_PIN, LOW);
        EPD_IT8951_ReadBusy();

        // Transfer the entire buffer in one bulk SPI call.
        DEV_SPI_WriteBuffer(spi_tx_buffer, 2 + (burst * 2));

        // Deassert chip select.
        DEV_Digital_Write(EPD_CS_PIN, HIGH);

        offset    += burst;
        remaining -= burst;
    }

    // End the load image command.
    EPD_IT8951_LoadImgEnd();
    EPD_IT8951_ReadBusy();
}

static void EPD_IT8951_HostAreaPackedPixelWrite_4bp_OneChunk(IT8951_Load_Img_Info* Load_Img_Info,
                                                              IT8951_Area_Img_Info* Area_Img_Info)
{
    // Calculate words per row.
    // For 4 bits per pixel: each pixel uses 0.5 byte.
    // Thus, for Area_W pixels: (Area_W / 2) bytes.
    // Each 16-bit word holds 2 bytes, so:
    //   words_per_row = (Area_W / 2) / 2 = Area_W / 4.
    UWORD words_per_row = (Area_Img_Info->Area_W * 4 / 8) / 2;
    UWORD total_rows = Area_Img_Info->Area_H;
    UWORD* Source_Buffer = (UWORD*)Load_Img_Info->Source_Buffer_Addr;
    
    // Total number of 16-bit words in the image.
    UDOUBLE total_words = (UDOUBLE)words_per_row * total_rows;
    // Total bytes: 2 bytes for preamble + 2 bytes per word.
    UDOUBLE total_bytes = 2 + (total_words * 2);

    // Dynamically allocate the transmit buffer.
    uint8_t *spi_tx_buffer = (uint8_t *)malloc(total_bytes);
    if (spi_tx_buffer == NULL)
    {
        printf("Error: Failed to allocate SPI transmit buffer of size %lu bytes.\n", (unsigned long)total_bytes);
        return;
    }

    // Build the buffer:
    // First 2 bytes: write command preamble (0x0000).
    spi_tx_buffer[0] = 0x00;
    spi_tx_buffer[1] = 0x00;

    // Next, convert the image data from 16-bit words to big-endian bytes.
    for (UDOUBLE i = 0; i < total_words; i++)
    {
        UWORD word = Source_Buffer[i];
        spi_tx_buffer[2 + (i * 2)]     = (word >> 8) & 0xFF;
        spi_tx_buffer[2 + (i * 2) + 1] = word & 0xFF;
    }

    // Set target memory address and start the load image command for the whole image.
    EPD_IT8951_SetTargetMemoryAddr(Load_Img_Info->Target_Memory_Addr);
    EPD_IT8951_LoadImgAreaStart(Load_Img_Info, Area_Img_Info);

    // Wait for the controller to be ready.
    EPD_IT8951_ReadBusy();

    // Assert CS and optionally check busy state again.
    DEV_Digital_Write(EPD_CS_PIN, LOW);
    EPD_IT8951_ReadBusy();

    // Send the entire buffer in one bulk SPI call.
    DEV_SPI_WriteBuffer(spi_tx_buffer, total_bytes);

    // Deassert chip select.
    DEV_Digital_Write(EPD_CS_PIN, HIGH);

    // End the load image command.
    EPD_IT8951_LoadImgEnd();
    EPD_IT8951_ReadBusy();

    free(spi_tx_buffer);
}

/******************************************************************************
function :	EPD_IT8951_HostAreaPackedPixelWrite_8bp
parameter:  
Precautions: Can't Packed Write
******************************************************************************/
static void EPD_IT8951_HostAreaPackedPixelWrite_8bp(IT8951_Load_Img_Info*Load_Img_Info,IT8951_Area_Img_Info*Area_Img_Info)
{
    UWORD Source_Buffer_Width, Source_Buffer_Height;

    UWORD* Source_Buffer = (UWORD*)Load_Img_Info->Source_Buffer_Addr;
    EPD_IT8951_SetTargetMemoryAddr(Load_Img_Info->Target_Memory_Addr);
    EPD_IT8951_LoadImgAreaStart(Load_Img_Info,Area_Img_Info);

    //from byte to word
    Source_Buffer_Width = (Area_Img_Info->Area_W*8/8)/2;
    Source_Buffer_Height = Area_Img_Info->Area_H;

    for(UDOUBLE i=0; i<Source_Buffer_Height; i++)
    {
        for(UDOUBLE j=0; j<Source_Buffer_Width; j++)
        {
            EPD_IT8951_WriteData(*Source_Buffer);
            Source_Buffer++;
        }
    }
    EPD_IT8951_LoadImgEnd();
}






/******************************************************************************
function :	EPD_IT8951_Display_Area
parameter:  
******************************************************************************/
static void EPD_IT8951_Display_Area(UWORD X,UWORD Y,UWORD W,UWORD H,UWORD Mode)
{
    UWORD Args[5];
    Args[0] = X;
    Args[1] = Y;
    Args[2] = W;
    Args[3] = H;
    Args[4] = Mode;
    //0x0034
    EPD_IT8951_WriteMultiArg(USDEF_I80_CMD_DPY_AREA, Args,5);
}



/******************************************************************************
function :	EPD_IT8951_Display_AreaBuf
parameter:  
******************************************************************************/
static void EPD_IT8951_Display_AreaBuf(UWORD X,UWORD Y,UWORD W,UWORD H,UWORD Mode, UDOUBLE Target_Memory_Addr)
{
    UWORD Args[7];
    Args[0] = X;
    Args[1] = Y;
    Args[2] = W;
    Args[3] = H;
    Args[4] = Mode;
    Args[5] = (UWORD)Target_Memory_Addr;
    Args[6] = (UWORD)(Target_Memory_Addr>>16);
    //0x0037
    EPD_IT8951_WriteMultiArg(USDEF_I80_CMD_DPY_BUF_AREA, Args,7); 
}



/******************************************************************************
function :	EPD_IT8951_Display_1bp
parameter:  
******************************************************************************/
static void EPD_IT8951_Display_1bp(UWORD X, UWORD Y, UWORD W, UWORD H, UWORD Mode,UDOUBLE Target_Memory_Addr, UBYTE Back_Gray_Val,UBYTE Front_Gray_Val)
{
    //Set Display mode to 1 bpp mode - Set 0x18001138 Bit[18](0x1800113A Bit[2])to 1
    EPD_IT8951_WriteReg(UP1SR+2, EPD_IT8951_ReadReg(UP1SR+2) | (1<<2) );

    EPD_IT8951_WriteReg(BGVR, (Front_Gray_Val<<8) | Back_Gray_Val);

    if(Target_Memory_Addr == 0)
    {
        EPD_IT8951_Display_Area(X,Y,W,H,Mode);
    }
    else
    {
        EPD_IT8951_Display_AreaBuf(X,Y,W,H,Mode,Target_Memory_Addr);
    }
    
    EPD_IT8951_WaitForDisplayReady();

    EPD_IT8951_WriteReg(UP1SR+2, EPD_IT8951_ReadReg(UP1SR+2) & ~(1<<2) );
}


/******************************************************************************
function :	Enhanced driving capability
parameter:  Enhanced driving capability for IT8951, in case the blurred display effect
******************************************************************************/
void Enhance_Driving_Capability(void)
{
    UWORD RegValue = EPD_IT8951_ReadReg(0x0038);
    Debug("The reg value before writing is %x\r\n", RegValue);

    EPD_IT8951_WriteReg(0x0038, 0x0602);

    RegValue = EPD_IT8951_ReadReg(0x0038);
    Debug("The reg value after writing is %x\r\n", RegValue);
}




/******************************************************************************
function :	Cmd1 SYS_RUN
parameter:  Run the system
******************************************************************************/
void EPD_IT8951_SystemRun(void)
{
    EPD_IT8951_WriteCommand(IT8951_TCON_SYS_RUN);
}


/******************************************************************************
function :	Cmd2 STANDBY
parameter:  Standby
******************************************************************************/
void EPD_IT8951_Standby(void)
{
    EPD_IT8951_WriteCommand(IT8951_TCON_STANDBY);
}


/******************************************************************************
function :	Cmd3 SLEEP
parameter:  Sleep
******************************************************************************/
void EPD_IT8951_Sleep(void)
{
    EPD_IT8951_WriteCommand(IT8951_TCON_SLEEP);
}


/******************************************************************************
function :	EPD_IT8951_Init
parameter:  
******************************************************************************/
IT8951_Dev_Info EPD_IT8951_Init(UWORD VCOM)
{
    IT8951_Dev_Info Dev_Info;

    EPD_IT8951_Reset();

    EPD_IT8951_SystemRun();

    EPD_IT8951_GetSystemInfo(&Dev_Info);
    
    //Enable Pack write
    EPD_IT8951_WriteReg(I80CPCR,0x0001);

    //Set VCOM by handle
    if(VCOM != EPD_IT8951_GetVCOM())
    {
        EPD_IT8951_SetVCOM(VCOM);
        Debug("VCOM = -%.02fV\n",(float)EPD_IT8951_GetVCOM()/1000);
    }
    return Dev_Info;
}


/******************************************************************************
function :	EPD_IT8951_Clear_Refresh
parameter:  
******************************************************************************/
void EPD_IT8951_Clear_Refresh(IT8951_Dev_Info Dev_Info,UDOUBLE Target_Memory_Addr, UWORD Mode)
{

    UDOUBLE ImageSize = ((Dev_Info.Panel_W * 4 % 8 == 0)? (Dev_Info.Panel_W * 4 / 8 ): (Dev_Info.Panel_W * 4 / 8 + 1)) * Dev_Info.Panel_H;
    UBYTE* Frame_Buf = malloc (ImageSize);
    memset(Frame_Buf, 0xFF, ImageSize);


    IT8951_Load_Img_Info Load_Img_Info;
    IT8951_Area_Img_Info Area_Img_Info;

    EPD_IT8951_WaitForDisplayReady();

    Load_Img_Info.Source_Buffer_Addr = Frame_Buf;
    Load_Img_Info.Endian_Type = IT8951_LDIMG_L_ENDIAN;
    Load_Img_Info.Pixel_Format = IT8951_4BPP;
    Load_Img_Info.Rotate =  IT8951_ROTATE_0;
    Load_Img_Info.Target_Memory_Addr = Target_Memory_Addr;

    Area_Img_Info.Area_X = 0;
    Area_Img_Info.Area_Y = 0;
    Area_Img_Info.Area_W = Dev_Info.Panel_W;
    Area_Img_Info.Area_H = Dev_Info.Panel_H;

    EPD_IT8951_HostAreaPackedPixelWrite_4bp(&Load_Img_Info, &Area_Img_Info, false);

    EPD_IT8951_Display_Area(0, 0, Dev_Info.Panel_W, Dev_Info.Panel_H, Mode);

    free(Frame_Buf);
    Frame_Buf = NULL;
}


/******************************************************************************
function :	EPD_IT8951_1bp_Refresh
parameter:
******************************************************************************/
void EPD_IT8951_1bp_Refresh(UBYTE* Frame_Buf, UWORD X, UWORD Y, UWORD W, UWORD H, UBYTE Mode, UDOUBLE Target_Memory_Addr, bool Packed_Write)
{
    IT8951_Load_Img_Info Load_Img_Info;
    IT8951_Area_Img_Info Area_Img_Info;

    EPD_IT8951_WaitForDisplayReady();

    Load_Img_Info.Source_Buffer_Addr = Frame_Buf;
    Load_Img_Info.Endian_Type = IT8951_LDIMG_L_ENDIAN;
    //Use 8bpp to set 1bpp
    Load_Img_Info.Pixel_Format = IT8951_8BPP;
    Load_Img_Info.Rotate =  IT8951_ROTATE_0;
    Load_Img_Info.Target_Memory_Addr = Target_Memory_Addr;

    Area_Img_Info.Area_X = X/8;
    Area_Img_Info.Area_Y = Y;
    Area_Img_Info.Area_W = W/8;
    Area_Img_Info.Area_H = H;


    //clock_t start, finish;
    //double duration;

    //start = clock();

    EPD_IT8951_HostAreaPackedPixelWrite_1bp(&Load_Img_Info, &Area_Img_Info, Packed_Write);

    //finish = clock();
    //duration = (double)(finish - start) / CLOCKS_PER_SEC;
	//Debug( "Write occupy %f second\n", duration );

    //start = clock();

    EPD_IT8951_Display_1bp(X,Y,W,H,Mode,Target_Memory_Addr,0xF0,0x00);

    //finish = clock();
    //duration = (double)(finish - start) / CLOCKS_PER_SEC;
	//Debug( "Show occupy %f second\n", duration );
}



/******************************************************************************
function :	EPD_IT8951_1bp_Multi_Frame_Write
parameter:  
******************************************************************************/
void EPD_IT8951_1bp_Multi_Frame_Write(UBYTE* Frame_Buf, UWORD X, UWORD Y, UWORD W, UWORD H,UDOUBLE Target_Memory_Addr, bool Packed_Write)
{
    IT8951_Load_Img_Info Load_Img_Info;
    IT8951_Area_Img_Info Area_Img_Info;

    EPD_IT8951_WaitForDisplayReady();

    Load_Img_Info.Source_Buffer_Addr = Frame_Buf;
    Load_Img_Info.Endian_Type = IT8951_LDIMG_L_ENDIAN;
    //Use 8bpp to set 1bpp
    Load_Img_Info.Pixel_Format = IT8951_8BPP;
    Load_Img_Info.Rotate =  IT8951_ROTATE_0;
    Load_Img_Info.Target_Memory_Addr = Target_Memory_Addr;

    Area_Img_Info.Area_X = X/8;
    Area_Img_Info.Area_Y = Y;
    Area_Img_Info.Area_W = W/8;
    Area_Img_Info.Area_H = H;
    
    EPD_IT8951_HostAreaPackedPixelWrite_1bp(&Load_Img_Info, &Area_Img_Info,Packed_Write);
}




/******************************************************************************
function :	EPD_IT8951_1bp_Multi_Frame_Refresh
parameter:  
******************************************************************************/
void EPD_IT8951_1bp_Multi_Frame_Refresh(UWORD X, UWORD Y, UWORD W, UWORD H,UDOUBLE Target_Memory_Addr)
{
    EPD_IT8951_WaitForDisplayReady();

    EPD_IT8951_Display_1bp(X,Y,W,H, A2_Mode,Target_Memory_Addr,0xF0,0x00);
}




/******************************************************************************
function :	EPD_IT8951_2bp_Refresh
parameter:  
******************************************************************************/
void EPD_IT8951_2bp_Refresh(UBYTE* Frame_Buf, UWORD X, UWORD Y, UWORD W, UWORD H, bool Hold, UDOUBLE Target_Memory_Addr, bool Packed_Write)
{
    IT8951_Load_Img_Info Load_Img_Info;
    IT8951_Area_Img_Info Area_Img_Info;

    EPD_IT8951_WaitForDisplayReady();

    Load_Img_Info.Source_Buffer_Addr = Frame_Buf;
    Load_Img_Info.Endian_Type = IT8951_LDIMG_L_ENDIAN;
    Load_Img_Info.Pixel_Format = IT8951_2BPP;
    Load_Img_Info.Rotate =  IT8951_ROTATE_0;
    Load_Img_Info.Target_Memory_Addr = Target_Memory_Addr;

    Area_Img_Info.Area_X = X;
    Area_Img_Info.Area_Y = Y;
    Area_Img_Info.Area_W = W;
    Area_Img_Info.Area_H = H;

    EPD_IT8951_HostAreaPackedPixelWrite_2bp(&Load_Img_Info, &Area_Img_Info,Packed_Write);

    if(Hold == true)
    {
        EPD_IT8951_Display_Area(X,Y,W,H, GC16_Mode);
    }
    else
    {
        EPD_IT8951_Display_AreaBuf(X,Y,W,H, GC16_Mode,Target_Memory_Addr);
    }
}




/******************************************************************************
function :	EPD_IT8951_4bp_Refresh
parameter:  
******************************************************************************/
void EPD_IT8951_4bp_Refresh(UBYTE* Frame_Buf, UWORD X, UWORD Y, UWORD W, UWORD H, bool Hold, UDOUBLE Target_Memory_Addr, bool Packed_Write)
{
    IT8951_Load_Img_Info Load_Img_Info;
    IT8951_Area_Img_Info Area_Img_Info;

    EPD_IT8951_WaitForDisplayReady();

    Load_Img_Info.Source_Buffer_Addr = Frame_Buf;
    Load_Img_Info.Endian_Type = IT8951_LDIMG_L_ENDIAN;
    Load_Img_Info.Pixel_Format = IT8951_4BPP;
    Load_Img_Info.Rotate =  IT8951_ROTATE_0;
    Load_Img_Info.Target_Memory_Addr = Target_Memory_Addr;

    Area_Img_Info.Area_X = X;
    Area_Img_Info.Area_Y = Y;
    Area_Img_Info.Area_W = W;
    Area_Img_Info.Area_H = H;

    /*struct timespec start, end;
    double elapsed_ms;

    // Record start time
    clock_gettime(CLOCK_MONOTONIC, &start);*/
    //EPD_IT8951_HostAreaPackedPixelWrite_4bp(&Load_Img_Info, &Area_Img_Info, Packed_Write);
    //EPD_IT8951_HostAreaPackedPixelWrite_4bp_PerRow(&Load_Img_Info, &Area_Img_Info); 
    EPD_IT8951_HostAreaPackedPixelWrite_4bp_Telegram(&Load_Img_Info, &Area_Img_Info);       
    //EPD_IT8951_HostAreaPackedPixelWrite_4bp_WholeImage(&Load_Img_Info, &Area_Img_Info);
    //EPD_IT8951_HostAreaPackedPixelWrite_4bp_OneChunk(&Load_Img_Info, &Area_Img_Info);

    /*// Record end time
    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculate elapsed time in milliseconds.
    elapsed_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                 (end.tv_nsec - start.tv_nsec) / 1000000.0;

    printf("Elapsed time HostAreaPackedPixelWrite: %f ms\n", elapsed_ms);*/

    if(Hold == true)
    {
        EPD_IT8951_Display_Area(X,Y,W,H, GC16_Mode);
    }
    else
    {
        EPD_IT8951_Display_AreaBuf(X,Y,W,H, GC16_Mode,Target_Memory_Addr);
    }
}


/******************************************************************************
function :	EPD_IT8951_8bp_Refresh
parameter:  
******************************************************************************/
void EPD_IT8951_8bp_Refresh(UBYTE *Frame_Buf, UWORD X, UWORD Y, UWORD W, UWORD H, bool Hold, UDOUBLE Target_Memory_Addr)
{
    IT8951_Load_Img_Info Load_Img_Info;
    IT8951_Area_Img_Info Area_Img_Info;

    EPD_IT8951_WaitForDisplayReady();

    Load_Img_Info.Source_Buffer_Addr = Frame_Buf;
    Load_Img_Info.Endian_Type = IT8951_LDIMG_L_ENDIAN;
    Load_Img_Info.Pixel_Format = IT8951_8BPP;
    Load_Img_Info.Rotate =  IT8951_ROTATE_0;
    Load_Img_Info.Target_Memory_Addr = Target_Memory_Addr;

    Area_Img_Info.Area_X = X;
    Area_Img_Info.Area_Y = Y;
    Area_Img_Info.Area_W = W;
    Area_Img_Info.Area_H = H;

    EPD_IT8951_HostAreaPackedPixelWrite_8bp(&Load_Img_Info, &Area_Img_Info);

    if(Hold == true)
    {
        EPD_IT8951_Display_Area(X, Y, W, H, GC16_Mode);
    }
    else
    {
        EPD_IT8951_Display_AreaBuf(X, Y, W, H, GC16_Mode, Target_Memory_Addr);
    }
}
