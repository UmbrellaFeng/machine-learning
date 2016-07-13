// cpp_speed_up.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"
#include "cpp_speed_up.h"
#include <stdio.h>
#include <math.h>
#include <float.h>

//���㵥�����
void __stdcall compute_cost_d(INT16 result[], INT16 left[], INT16 right[], INT16 strides[], INT16 shapes[]) {
	//row length
	int row_length = shapes[0];
	//column length
	int column_length = shapes[1];
	//row and column size
	int S0 = strides[0] / sizeof(INT16);
	int S1 = strides[1] / sizeof(INT16);
	for (int row = 0; row < row_length; row ++) {
		for (int column = 0; column < column_length; column ++) {
			int pos = row*S0 + column*S1;
			if (left[pos] > right[pos]) {
				result[pos] = left[pos] - right[pos];
			}
			else {
				result[pos] = right[pos] - left[pos];
			}
		}
	}
}

//���㵥����� BT�汾
void __stdcall compute_cost_bt_d(INT16 result[], INT16 left[], INT16 right[], INT16 strides[], INT16 shapes[]) {
	//row length
	int row_length = shapes[0];
	//column length
	int column_length = shapes[1];
	//row and column size
	int S0 = strides[0] / sizeof(INT16);
	int S1 = strides[1] / sizeof(INT16);
	for (int row = 0; row < row_length; row++) {
		INT16 * left_row = &(left[row*S0]);
		INT16 * right_row = &(right[row*S0]);
		INT16 * result_row = &(result[row*S0]);
		for (int column = 0; column < column_length; column++) {
			int pos =column*S1;
			int right_pixel_l, right_pixel_r, right_pixel;
			int right_pixel_min, right_pixel_max;
			right_pixel = right_row[pos];
			if (column == 0) {
				right_pixel_l = right_row[pos];
				right_pixel_r = right_row[pos+1];
			}
			else if (column == column_length - 1) {
				right_pixel_l = right_row[pos - 1];
				right_pixel_r = right_row[pos];
			}
			else {
				right_pixel_l = right_row[pos - 1];
				right_pixel_r = right_row[pos + 1];
			}
			//�����е�
			right_pixel_l = (right_pixel_l + right_pixel) / 2;
			right_pixel_r = (right_pixel_r + right_pixel) / 2;
			//����ֵ����С���ֵ
			right_pixel_min = min(right_pixel_l, right_pixel_r);
			right_pixel_min = min(right_pixel_min, right_pixel);
			right_pixel_max = max(right_pixel_l, right_pixel_r);
			right_pixel_max = max(right_pixel_max, right_pixel);
			//���ս��
			int diff = max(0, left_row[pos] - right_pixel_max);
			diff = max(diff, right_pixel_min - left_row[pos]);
			result_row[pos] = diff;
		}
	}
}

//���۾ۺ�
void __stdcall aggregate_cost(INT32 result[], INT16 diff[],INT32 diff_strides[], INT32 result_strides[], INT16 shapes[], INT16 window_size) {
	//deep length
	int d_max = shapes[0];
	//row length
	int row_length = shapes[1];
	//column length
	int column_length = shapes[2];
	//deep size
	int S_deep = diff_strides[0] / sizeof(INT16);
	//row size
	int S_row = diff_strides[1] / sizeof(INT16);
	//column size
	int S_column = diff_strides[2] / sizeof(INT16);


	//result row size
	int S_row_r = result_strides[0] / sizeof(INT32);
	//result column size
	int S_column_r = result_strides[1] / sizeof(INT32);
	//result deep size
	int S_deep_r = result_strides[2] / sizeof(INT32);
	//printf("row_length:%d column_length:%d d_max:%d\r\n", row_length,column_length,d_max);
	for (int d = 0; d < d_max; d++) {
		//printf("d:%d\r\n", d);
		INT16* diff_this_deep = &(diff[S_deep*d]);
		for (int row = 0; row < row_length; row++) {
			//��ȷ���쳣�߽�
			int top=0, bottom = row_length;
			//������� ����ȷ���߽�
			if (row - window_size / 2 > 0) {
				top = row - window_size / 2;
			}
			if (row + window_size / 2 + 1 <= row_length) {
				bottom = row + window_size / 2 + 1;
			}
			for (int column = 0; column < column_length; column++) {
				//��ȷ���쳣�߽�
				int left = 0,right = column_length;
				//������� ����ȷ���߽�
				if (column - window_size / 2 > 0) {
					left = column - window_size / 2;
				}
				if (column + window_size / 2 + 1 <= column_length) {
					right = column + window_size / 2 + 1;
				}
				//��� SAD
				INT32 sad = 0;
				for (int i = top; i < bottom; i++) {
					for (int j = left; j < right; j++) {
						sad += diff_this_deep[i*S_row + j*S_column];
					}
				}
				//��һ��
				INT32 sad_normal = 100 * sad / (bottom - top) / (right - left);
				result[row*S_row_r + column*S_column_r + d*S_deep_r] = sad_normal;
			}
		}
	}
}
void __stdcall DP_search_forward(INT16 result[], float cost[], INT16 sad_row[], INT32 column_length, INT32 d_max, float p) {
	//��һ������ û��Լ����
	for (int d = 0; d < d_max; d++) {
		//���۵��� ������
		float cost_t = sad_row[0 * d_max + d];
		cost[0 * d_max + d] = cost_t;
	}
	//����һ����������
	for (int column = 1; column < column_length; column++) {
		//���������Ӳ�
		for (int d = 0; d < d_max; d++) {
			int pos = column * d_max + d;
			//�ϴε�����Ӳ�
			int min_last = 0;
			//������۽��
			float cost_result = DBL_MAX;
			//��һ������Ӳ�
			for (int last_pos = 0; last_pos < d_max; last_pos++) {
				//Լ����
				int disparity_diff = abs(d - last_pos);
				int cost_disparity = p*disparity_diff;
				//������Ϊ�̶�ֵ ȷ����Ե
				if (disparity_diff > 2) {
					cost_disparity = 3 * disparity_diff;
				}
				// ���۵��� ������ + Լ��ϵ��*Լ����
				float cost_now = sad_row[pos] + cost_disparity;
				//���ۺϼ�
				float cost_sum = cost_now + cost[(column-1) * d_max + last_pos];
				//ȡ����Сֵ
				if (cost_sum < cost_result) {
					cost_result = cost_sum;
					min_last = last_pos;
				}
			}
			result[pos] = min_last;
			cost[pos] = cost_result;
		}
	}
}

