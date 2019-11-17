
typedef volatile struct _PIO
{
	unsigned int PER;			/** PIO Enable register (Write-only)*/
	unsigned int PDR;			/** PIO Disable register (Write-only)*/
	unsigned int PSR;			/** PIO Status register (Read-only)*/
	unsigned int Reserved0;		/** Unused register*/
	unsigned int OER;			/** Output Enable register (Write-only)*/
	unsigned int ODR;			/** Output Disable register (Write-only)*/
	unsigned int OSR;			/** Output Status register (Read-only)*/
	unsigned int Reserved1;		/** Unused register*/
	unsigned int IFER;			/** Glitch Input filter Enable (Write-only)*/
	unsigned int IFDR;			/** Glitch Input filter Disable (Write-only)*/
	unsigned int IFSR;			/** Glitch Input filter Status (Read-only)*/
	unsigned int Reserved2;		/** Unused register*/
	unsigned int SODR;			/** Set Output Data register (Write-only)*/
	unsigned int CODR; 			/** Clear Output Data register (Write-only)*/
	unsigned int ODSR; 			/** Output Data Status register (Read-only)*/
	unsigned int PDSR; 			/** Pin Data Status register (Read-only)*/
	unsigned int IER; 			/** Interrupt Enable register (Write-only)*/
	unsigned int IDR; 			/** Interrupt Disable register (Write-only)*/
	unsigned int IMR; 			/** Interrupt Mask register (Write-only)*/
	unsigned int ISR; 			/** Interrupt Status register (Read-only)*/
	unsigned int MDER; 			/** Multi-driver Enable (Write-only)*/
	unsigned int MDDR; 			/** Multi-driver Disable (Write-only)*/
	unsigned int MDSR; 			/** Multi-driver Status (Read-only)*/
	unsigned int Reserved3; 	/** Unused register*/
	unsigned int PUDR; 			/** Pull-up Disable register (Write-only)*/
	unsigned int PUER; 			/** Pull-up Enable register (Write-only)*/
	unsigned int PUSR; 			/** Pull-up Status register (Read-only)*/
	unsigned int Reserved4; 	/** Unused register*/
	unsigned int ASR; 			/** Peripheral A select(Write-only)*/
	unsigned int BSR; 			/** Peripheral B select(Write-only)*/
	unsigned int ABSR; 			/** Peripheral AB Status(Read-only)*/
	unsigned int Reserved5[9]; 	/** Unused register */
	unsigned int OWER; 			/** Output write enable (Write-only)*/
	unsigned int OWDR; 			/** Output write disable (Write-only)*/
	unsigned int OWSR; 			/** Output write Status (Read-only)*/
}PIO;

typedef volatile struct _AIC
{
	unsigned int SMR[32]; 		/** Source mode register (Read-Write)*/
	unsigned int SVR[32]; 		/** Source vector register (Read-Write)*/
	unsigned int IVR; 			/** Interrupt vector register (Read-only)*/
	unsigned int FVR; 			/** Fast Interrupt vector register (Read-only)*/
	unsigned int ISR; 			/** Interrupt status register (Read-only)*/
	unsigned int IPR; 			/** Interrupt pending register (Read-only)*/
	unsigned int IMR; 			/** Interrupt mask register (Read-only)*/
	unsigned int CISR; 			/** Core Interrupt status register (Read-only)*/
	unsigned int Reserved1[2]; 	/** Unused register*/
	unsigned int IECR; 			/** Interrupt enable command register (Write-only)*/
	unsigned int IDCR; 			/** Interrupt disable command register (Write-only)*/
	unsigned int ICCR; 			/** Interrupt clear command register (Write-only)*/
	unsigned int ISCR; 			/** Interrupt set command register (Write-only)*/
	unsigned int EICR; 			/** End of Interrupt command register (Write-only)*/
	unsigned int SPUR; 			/** Spurious Interrupt vector register (Read-Write)*/
	unsigned int DCR; 			/** Debug control register (Read-Write)*/
	unsigned int Reserved2; 	/** Unused register*/
	unsigned int FFER; 			/** Fast Forcing enable register (Write-only)*/
	unsigned int FFDR; 			/** Fast Forcing disable register (Write-only)*/
	unsigned int FFSR; 			/** Fast Forcing status register (Write-only)*/
}AIC;

typedef volatile struct _TCCHAN
{
	unsigned int CCR; 				/** Channel Control Register (Write-only) */
	unsigned int CMR; 				/** Channel Mode Register (Read-Write) */
	unsigned int Reserved1[2]; 		/** Unused register */
	unsigned int CV; 				/** Counter Value (Read-only) */
	unsigned int RA; 				/** Register A (Read-Write) */
	unsigned int RB; 				/** Register B (Read-Write) */
	unsigned int RC; 				/** Register C (Read-Write) */
	unsigned int SR; 				/** Status Register (Read-only) */
	unsigned int IER; 				/** Interrupt Enable Register (Write-only) */
	unsigned int IDR; 				/** Interrupt Disable Register (Write-only) */
	unsigned int IMR; 				/** Interrupt Mask Register (Read-only) */
	unsigned int Reserved2[4]; 		/** Unused register */
}TCCHAN;

typedef volatile struct _TC
{
	TCCHAN Channel_0;
	TCCHAN Channel_1;
	TCCHAN Channel_2;
	unsigned int BCR; /** Block Control Register (Write-only)*/
	unsigned int BMR; /** Block Mode Register (Read-Write)*/
}TC;

#define STARTUP \
unsigned int _FIQtmp; \
int fd = open("/dev/mem", O_RDWR | O_SYNC); \
int fd2 = open("/proc/FIQ", O_RDWR | O_SYNC); \
char* mptr = mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0xFFFFF000); \
char* pptr = mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0xFFFA0000); \
if(mptr == MAP_FAILED || pptr == MAP_FAILED){ \
close(fd); \
close(fd2); \
exit(1); \
} \
pioa = (PIO*)(mptr + 0x400); \
aic = (AIC*)(mptr); \
tc = (TC*)(pptr); \
_FIQtmp = (unsigned int)FIQ_handler; \
write(fd2, &_FIQtmp, sizeof(unsigned int)); \
_FIQtmp = fcntl(STDIN_FILENO, F_GETFL, 0); \
_FIQtmp |= O_NONBLOCK; \
fcntl(STDIN_FILENO, F_SETFL, _FIQtmp)

#define CLEANUP \
_FIQtmp = fcntl(STDIN_FILENO, F_GETFL, 0); \
_FIQtmp &= O_NONBLOCK; \
fcntl(STDIN_FILENO, F_SETFL, _FIQtmp); \
munmap(mptr, 0x1000); \
munmap(pptr, 0x1000); \
close(fd); \
close(fd2)

#define DISABLE_FIQ \
{ \
unsigned int __Reg_save; \
asm("mrs %0, cpsr;" \
"orr %0, %0, #0x40;" \
"msr cpsr_c, %0;" \
:"=r" (__Reg_save) \
:"r"(__Reg_save) \
);}

#define ENABLE_FIQ \
{ \
unsigned int __Reg_save; \
asm("mrs %0, cpsr;" \
"bic %0, %0, #0x40;" \
"msr cpsr_c, %0;" \
:"=r" (__Reg_save) \
:"r"(__Reg_save) \
);}
