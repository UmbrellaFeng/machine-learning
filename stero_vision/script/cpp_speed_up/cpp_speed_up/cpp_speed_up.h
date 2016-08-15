#pragma once
//������ 16
#define SUB_PIXEL_LEVEL 16
//��������ͼ
extern "C" _declspec(dllexport) void __stdcall integral(INT32 integral_result[], INT32 image[], INT32 shapes[]);
//���㵥�����
extern "C" _declspec(dllexport) void __stdcall compute_cost_d(INT16 result[], INT16 left[], INT16 right[], INT16 strides[], INT16 shapes[]);
//���㵥����� BT�汾
extern "C" _declspec(dllexport) void __stdcall compute_cost_bt_d(INT16 result[], INT16 left[], INT16 right[], INT16 strides[], INT16 shapes[]);
//���۾ۺ�
extern "C" _declspec(dllexport) void __stdcall aggregate_cost(INT32 result[], INT16 diff[], const INT32 diff_strides[], const INT32 result_strides[], const INT16 shapes[], const INT16 window_size);
//��̬�滮
extern "C" _declspec(dllexport) void __stdcall DP_search_forward(INT16 result[], float cost[], const INT16 sad_row[], const INT32 column_length, const  INT32 d_max, const float p);
//��̬�滮 �򻯰�
extern "C" _declspec(dllexport) void __stdcall DP_search_forward2(INT16 result[], float cost[], const  INT16 sad_row[], const INT32 column_length, const INT32 d_max, const float p);
//�Ӳ����
extern "C" _declspec(dllexport) void __stdcall get_result(INT16 result[], const INT32 sad_diff[], const INT32 strides[], const INT32 shapes[]);
//��������
extern "C" _declspec(dllexport) int __stdcall subpixel_calculator(int d, int f_d, int f_d_l, int f_d_r);
//�����Ӳ���
extern "C" _declspec(dllexport) void __stdcall left_right_check(INT16 result[], INT16 left[], INT16 right[], const INT32 strides[], const INT32 shapes[]);
/*-------------census-------------*/
//����census
extern "C" _declspec(dllexport) void __stdcall get_census(BOOLEAN result[], const INT16 image[], const INT32 strides[], const INT32 shapes[], const INT32 window_size);
//��������bool����hamming����
extern "C" _declspec(dllexport) INT16 __stdcall get_hamming_distance(const BOOLEAN census1[], const BOOLEAN census2[], const int len);
//���㵥����� census�汾
extern "C" _declspec(dllexport) void __stdcall compute_cost_census_d(INT16 result[], BOOLEAN left[], BOOLEAN right[], const INT16 strides[], const INT16 shapes[]);