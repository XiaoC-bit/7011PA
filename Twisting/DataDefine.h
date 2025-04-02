#ifndef _DATA_DEFINE_H
#define _DATA_DEFINE_H
#include <qstring.h>

struct Config{
	int debugPort;//前端调试端口
	int webSocketPort;//websocket端口
	int frontPort;//前端URL端口
	QString frontExeName;//前端启动进程名称
};



typedef struct
{
	// ============================================================
	//                   : 0x08000 ~ 0x08FF
	// ============================================================
	//    STRU_ReadU65_EXT_0800 stru_ReadU65ext0800;
	// ============================================================
	//                   : 0x0956 ~ 0x095C
	// ============================================================
	unsigned int mui_TEST_TM_MS,
		mui_TEST_TIMER,
		mui_HOLD_TIMER;
	// ============================================================
	//                   : 0x0980 ~ 0x09A0
	// ============================================================
	double md_TEMP_1,			 // 放1 陪ボ (虫:'C)
		md_TEMP_2,				 // 放1 陪ボ (虫:'C)
		md_S_IR_1,				 // 陪ボ DA の
		md_AD_1_PID,			 // 陪ボPID_ZERO[0] 秖0翴北秖
		md_F_PP_N,				 // 畃魁虫: N
		md_L_PP_N;				 // 魁虫: mm
	unsigned int mui_DF_CYCLE_1; // 陪ボ猧Ω计
	double md_CY_F_PP,			 // セ秅戳秖 + 畃
		md_CY_F_PP_N,			 // セ秅戳秖 - 畃
		md_CY_L_PP,				 // セ㏄戳 + 畃
		md_CY_L_PP_N;			 // セ㏄戳 - 畃
	unsigned int mui_CT_1,		 // ヘ玡竒磅︽癹伴 1 Ω计
		mui_CT_2,				 //         "          2 Ω计
		mui_CT_3;				 //         "          3 Ω计
	// ============================================================
	//                   : 0x0A04 ~ 0x0A0F
	// ============================================================
	// : 0x0A04 ~ 0x0A18
	unsigned int mui_MACHINE_TYPE, //  MACHINE_TYPE
		mui_CH1_FLAG,			   // CH1_FLAG
		mui_CH2_ADD,			   // CH2_ADD
		mui_CH2_FLAG,			   // CH2_FLAG
		mui_SYS_FLAG1,			   // SYS_FLAG1
		mui_DIR_FLAG;			   // DIR_FLAG
	// --------------------------------------------
	// : 0x0A18 ~ 0x0A30
	unsigned int mui_IO_1_DIR, // IO1_DIR  IO1 IN
		mui_SYNC_ZERO_FLAG,	   // SYNC_ZERO_FLAG
		mui_EN_PID,			   // EN_PID
		mui_BREAK_TIMES;	   //
	double md_ST_LOAD_LEVEL,   // 
		md_BREAK_LEVEL,		   //
		md_S_RATE,
		md_SITA_COMP,
		md_S_STANDARD,
		md_MOTOR_STABLE,
		md_MOLD_DELAY,
		md_S_RATE_MEM,
		md_SITA_COMP_MEM,
		md_X_RATE,
		md_YZ_RATE;
	// --------------------------------------------
	// : 0x0A58 ~ 0x0A5F
	double md_LIM_1_FORCE_S_N, //
		md_LIM_1_LENGTH_S_mm;  //
	// ============================================
	// : 0x0A80 ~ 0x0AAE (DF8000)
	double md_LIM_1_FORCE_N,		 //
		md_LIM_1_LENGTH_mm;			 //
	unsigned int mui_DF_ABS_L_LIM_P, // (ABS_AD2)
		mui_DF_ABS_L_LIM_N,			 // (ABS_AD2)
		mui_DF_ABS_F_LIM_P,			 // (ABS_AD2)
		mui_DF_ABS_F_LIM_N;			 // (ABS_AD2)
	float mf_TEMP_ABORT,			 //
		mf_TEMP_FAN_ON,				 //
		mf_TEMP_FAN_OFF;			 //
	unsigned int mui_DF_EMG_MODE,	 //
		mui_DF_POS_IMP;				 //
	// --------------------------------------------
	// : 0x0AF0 ~ 0x0AFC
	unsigned int mui_PANEL_TYPE, // 
		mui_PANEL_VER,			 // 
		mui_PA_DATE,			 //  (Α: DD:MM:YYYY)
		mui_MA_TYPE,			 //  (= 6500)
		mui_MA_VER,				 // 
		mui_MA_DATE;			 //  (Α: DD:MM:YYYY)
} STRU_ReadU65_EXT;

struct AD_Struct
{
	/*double Capacity,Value,Ad_Gain;
	unsigned long ABS_AD;
	bool TenDir,CompDir,BendDir;
	double CalAD[21],CalAbs[21];//Page5 - Page 18  20:﹍タ翴
	unsigned short K16Max,K16Min,K16N,ID;*/

	double Cap, Value, AdGain, SimAdRate, Impact_Level, Filter;
	double CalN[71], CalAD[71], CalAbs[71]; // Page5 - Page 4A  35:﹍タ翴

	unsigned long ABS_AD;
	unsigned short K16Max, K16Min, K16N, ID;
	short Pos;
	bool TenDir, CompDir, BendDir, Dir;
	int K2Time;
};


struct ReadU65Struct
{
	ReadU65Struct() {

	}
	int Version;
	int  PC_KEY_CT, PC_KEY, MOVE_FLAG1, MOVE_FLAG2, MOVE_FLAG3, U65_MODE, U65_MSG,
		U65_MSG2, U65_MSG3, IO1_OUT, IO1_IN, REC_NO1, REC_NO2, REC_COMP;
	unsigned int Sys_Flag2, Axe_1_F_No, Axe_1_L_No;
	unsigned int PC_TEST_1, PC_TEST_2, PC_TEST_3, uI[10];
	unsigned int Sys_Flag1, DIR_FLAG, Df_Set_No, Zero, Temp1, Temp2;
	unsigned short REMOTE_KEY, REMOVE_VR, xUsbCrc, PcWriteCt, uS[9];
	double TEST_TIMER, HOLD_TIMER, X_ABS, X_mm, YZ_mm, F_PP, L_PP, SimAdRate[6], X_SPEED, YZ_SPEED, x[8];
	double Wave[1024];

	double TEST_TM_MS;//小数点后的秒数

	unsigned char CartDetail[15][16], MasterDetail[8], PannelDetail[8];
	unsigned short IP[8];


	float SITA;//S*  的θ 角
	float sStar;

	float sQuotation;
	float sDoubleQuotation;

	float upperTemp, lowerTemp;

	float AD_2;//P 发泡力

	STRU_ReadU65_EXT stru_ReadU65ext;
	//EJParam_Struct EJParam_Upper, EJParam_Lower;
	AD_Struct Ad[6], LvdtX[6];
	//STRU_M5000_SET stru_M5000_SET_def;
	//PID_Bank_Struct RAM_PID[15];
	//STRU_M5000_SET M5000_Set[100]; // 0 ~ 99
};

//扭转机专用的及时资料区
struct TwistingData {
	double torque;//扭矩
	double angle;//角度
	double axialDisplacement;//轴向位移
	double twistCount;
	double testTimer;
};

struct U65RawData {
	float sStar;
	float sQuotation;
	float sDoubleQuotation;
	float tanPA;
	float angle;
	float P;
	float upperTemp;//上模温度
	float lowerTemp;//下模温度
	float time;
	int stepNo;//当前执行的步骤，门尼机使用
	ReadU65Struct U65Info;

	TwistingData twistingData;

	U65RawData() {
		sStar = 0.0;
		sQuotation = 0.0;
		sDoubleQuotation = 0.0;
		tanPA = 0.0;
		P = 0.0;
		upperTemp = 0.0;
		lowerTemp = 0.0;
		time = 0.0;
		angle = 0.0;
		stepNo = 0;
	}

	bool isTesting() const {
		if (U65Info.U65_MODE == 2) {
			return false;
		}
		else if (U65Info.U65_MODE == 3) {
			if (U65Info.U65_MSG < 10) {
				//马达没启动
				return false;
			}
			else {
				return true;
			}
		}
		else if (U65Info.U65_MODE == 11) {
			return true;
		}
		return false;
	}
};
Q_DECLARE_METATYPE(U65RawData);




struct  DF_SET
{
	unsigned int mui_GroupNo,     // 测试步骤 本组组别（0~99）
		mui_DISCARD_SAMPLE,  // SWEEP舍弃样本数量
		mui_TEST_MODE,     // 0:结束循环 1:MDR(定角度) 2:MDR2(定扭矩) 3：RELAX 4：SWEEP 5：DELAY
		mui_SAMPLE,       // SWEEP取样样本数量
		mui_TEMP_TOLERANCE,//温度误差 单位0.01度
		mui_IR_TEMP,       // 指令温度  单位：0.01度
		mui_MDR_FLAG,      //BIT_0,1=MDR结束条件   00：两者之一到结束 01:测试时间到 10:MH时间到 11:测试跟MH时间同时到达			 BIT_2,3=MDR2温升模式
		mui_STABLE_TIME;  //等待温度稳定时间 单位：秒

	float       md_IR_CPM,       // 指定CPM（RPA8000）
		md_IR_ANG;       // 指定角度或扭力（MDR2）
	int          mi_CONTROL_TIME, // MDR 结束条件： MH持平时间   SWEEP: 稳定时间  DELAY： 延迟时间
		mi_TEST_EndTime; // 结束条件： 测试时间  HH/MM/MSMS

	// --------------------------------------------
	DF_SET()
	{
		mui_GroupNo = 0;
		mui_DISCARD_SAMPLE = 0;
		mui_TEST_MODE = 1;
		mui_SAMPLE = 100;
		mui_TEMP_TOLERANCE = 30;
		mui_IR_TEMP = 2500;
		mui_MDR_FLAG = 0;
		mui_STABLE_TIME = 3;
		md_IR_CPM = 0;
		md_IR_ANG = 0.5;
		mi_CONTROL_TIME = 0;
		mi_TEST_EndTime = 360;
	}
};


#endif
