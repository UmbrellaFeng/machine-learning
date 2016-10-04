// cpp_speed_up.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"
#include "cpp_speed_up.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

//��������ͼ
void __stdcall integral(INT32 integral_result[], INT16 image[], int row_length,int column_length) {
	//�л���
	INT32 *rowSum = new INT32[column_length]; // sum of each column

	// �����һ�л���
	for (int column = 0; column<column_length; column++) {
		rowSum[column] = image[column];
		integral_result[column] = image[column];
		if (column>0) {
			integral_result[column] += integral_result[column - 1];
		}
	}
	for (int row = 1; row<row_length; row++) {
		int offset = row*column_length;
		// ÿ������ = ��һ�� + ������
		rowSum[0] += image[offset];
		integral_result[offset] = rowSum[0];
		for (int column = 1; column < column_length; column++) {
			int pos = offset + column;
			// ������ 
			rowSum[column] += image[pos];
			integral_result[pos] = rowSum[column] + integral_result[pos - 1];
		}
	}
	delete(rowSum);
}
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
void __stdcall aggregate_cost(INT32 result[], INT16 diff[], const INT32 diff_strides[], const INT32 result_strides[], const INT16 shapes[],const INT16 window_size) {
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
		//�õ�����ͼ
		INT32* integral_result = new INT32[row_length*column_length];
		integral(integral_result, diff_this_deep, row_length, column_length);
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
				/*for (int i = top; i < bottom; i++) {
					for (int j = left; j < right; j++) {
						sad += diff_this_deep[i*S_row + j*S_column];
					}
				}*/
				//���û���ͼ
				sad = integral_result[(bottom - 1)*column_length + (right - 1)];

				if (left >0) {
					sad -= integral_result[(bottom - 1)*column_length + (left - 1)];
				}
				if (top > 0&& left >0) {
					sad += integral_result[(top-1)*column_length + (left - 1)];
				}
				if (top > 0) {
					sad -= integral_result[(top-1)*column_length + (right - 1)];
				}
				//��һ��
				INT32 sad_normal = 100 * sad / (bottom - top) / (right - left);
				result[row*S_row_r + column*S_column_r + d*S_deep_r] = sad_normal;
			}
		}
		delete(integral_result);
	}
}
//��̬�滮
void __stdcall DP_search_forward(INT16 result[], float cost[], const INT16 sad_row[], const INT32 column_length, const  INT32 d_max, const float p) {
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
//��̬�滮 �򻯰�
void __stdcall DP_search_forward2(INT16 result[], float cost[], const  INT16 sad_row[], const INT32 column_length, const INT32 d_max,const float p) {
	
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
			for (int last_pos = (d - 1<0?0:d-1); last_pos < (d + 2<d_max?d+2:d_max); last_pos++) {
				//Լ����
				int disparity_diff = abs(d - last_pos);
				int cost_disparity = p*disparity_diff;
				// ���۵��� ������ + Լ��ϵ��*Լ����
				float cost_now = sad_row[pos] + cost_disparity;
				//���ۺϼ�
				float cost_sum = cost_now + cost[(column - 1) * d_max + last_pos];
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

//�����Ӳ���
void __stdcall left_right_check(INT16 result[], INT16 left[], INT16 right[], const INT32 strides[], const INT32 shapes[]) {
	//row length
	int row_length = shapes[0];
	//column length
	int column_length = shapes[1];
	//row and column size
	int S0 = strides[0] / sizeof(INT16);
	int S1 = strides[1] / sizeof(INT16);
	//printf("shapes %d, %d\r\n", row_length, column_length);
	//printf("strides %d, %d\r\n", S0, S1);
	for (int row = 0; row < row_length; row++) {
		for (int column = 0; column < column_length; column++) {
			int pos_l = row*S0 + column*S1;
			//���ֵ
			int disparity_left = left[pos_l];
			//�Ҳ�λ��
			int column_r = column - (disparity_left / SUB_PIXEL_LEVEL);
			if (column_r < 0) {
				//result[pos_l] = 16;
				continue;
			}
			//�Ҳ��Ӳ�
			int pos_r = row*S0 + column_r*S1;
			int disparity_right = right[pos_r];
			//�õ��ڵ� / ���ȶ���
			int diff = abs(disparity_left - disparity_right);
			if (diff > SUB_PIXEL_LEVEL / 2) {
				result[pos_l] = diff;
			}
		}
	}
}

//�Ӳ����
void __stdcall post_processing(INT16 result[], const INT16 left_right_result[], const INT32 strides[], const INT32 shapes[],const INT32 window_size,const INT32 d_max) {
	//row length
	int row_length = shapes[0];
	//column length
	int column_length = shapes[1];
	//row and column size
	int S0 = strides[0] / sizeof(INT16);
	int S1 = strides[1] / sizeof(INT16);
	int* vote = new int[d_max * 16];
	for (int row = 0; row < row_length; row++) {
		for (int column = 0; column < column_length; column++) {
			int pos = row*S0 + column*S1;
			if (left_right_result[pos]) {
				//��ɱ�־
				bool finish_flag = false;
				//�õ�����
				int top = row - window_size / 2;
				if (top < 0) { top = 0; }
				int bottom = row + window_size / 2 + 1;
				if (bottom > row_length) { bottom = row_length; }
				int left = column - window_size / 2;
				if (left < 0) { left = 0; }
				int right = column + window_size / 2 + 1;
				if (right > column_length) { right = column_length; }
				while (!finish_flag) {
					memset(vote, 0, sizeof(int)*d_max * 16);
					//vote����
					for (int i = top; i < bottom; i++) {
						int offset_base = i*S0;
						for (int j = left; j < right; j++) {
							if (left_right_result[offset_base + j*S1] <= 8) {
								vote[result[offset_base + j*S1]] += 1;
							}
						}
					}
					//��ȡ����±�
					INT16 max = 0;
					INT16 max_l = 0;
					for (int i = 1; i < d_max * 16; i++) {
						if (vote[i] > vote[max]) {
							max = i;
							max_l = max;
						}
					}
					INT32 SUM = vote[max] + vote[max_l];
					int sum_size = window_size*window_size / 2;
					//������ֵ
					if (SUM > sum_size) {
						INT32 P1 = 100 * vote[max] / (SUM);
						INT32 P2 = 100 - P1;
						//��д���
						result[pos] = (max*P1 + max_l*P2) / 100;
						finish_flag = true;
					}
					else {
						if (top > 0) { top -= 1; }
						if (bottom < row_length) { bottom += 1; }
						if (left > 0) { left -= 1; }
						if (right < column_length) { right += 1; }
					}
				}
			}
		}
	}
}

//�Ӳ����
void __stdcall get_result(INT16 result[], const INT32 sad_diff[], const INT32 strides[], const INT32 shapes[] ) {
	//row length
	int row_length = shapes[0];
	//column length
	int column_length = shapes[1];
	//disparity length
	int d_length = shapes[2];
	//row and column size
	int S0 = strides[0] / sizeof(INT32);
	int S1 = strides[1] / sizeof(INT32);
	int S2 = strides[2] / sizeof(INT32);
	//printf("%d,%d,%d\r\n", row_length, column_length, d_length);
	//printf("%d,%d,%d\r\n", S0, S1, S2);
	for (int row = 0; row < row_length; row++) {
		for (int column = 0; column < column_length; column++) {
			int pos = row*S0 + column*S1;
			int min_disparity = 0;
			for (int d = 1; d < d_length; d++) {
				int sad_diff_pos = pos + d*S2;
				if (sad_diff[sad_diff_pos] < sad_diff[pos + min_disparity*S2]) {
					min_disparity = d;
				}
			}
			int result_pos = row*column_length + column;
			//��������
			min_disparity = subpixel_calculator(min_disparity,
				sad_diff[pos + min_disparity*S2],
				sad_diff[pos + (min_disparity - 1)*S2],
				sad_diff[pos + (min_disparity + 1)*S2]);
			result[result_pos] = min_disparity;
		}
	}
}

//��������
int __stdcall subpixel_calculator(int d,int f_d,int f_d_l,int f_d_r) {
	d *= SUB_PIXEL_LEVEL;
	//2a = 2(f(d+)+d(d-)-2f(d))
	int a = (f_d_r + f_d_l - f_d);
	if (a != 0) {
		//b = f(d+)-f(d-)
		int b = (f_d_r - f_d_l);
		//d = -b/2a
		d = d - (SUB_PIXEL_LEVEL * b) / (2 * a);
		if (d < 0) {
			return 0;
		}
		else {
			return d;
		}
	}
	else {
		return d;
	}
}
//��������
void __stdcall low_texture_detection(INT16 row_result[], INT16 column_result[], const INT16 image[], const INT32 strides[], const INT32 shapes[], const INT32 window_size, const INT32 texture_range) {
	//row length
	int row_length = shapes[0];
	//column length
	int column_length = shapes[1];
	//��ֽ��
	INT16 * difference_integral = new INT16[row_length*column_length];
	//row and column size
	const int S0 = strides[0] / sizeof(INT16);
	const int S1 = strides[1] / sizeof(INT16);
	//printf("shapes %d, %d\r\n", row_length, column_length);
	//printf("strides %d, %d\r\n", S0, S1);
	//�õ���ֽ�� �� �� Ϊһά�ռ�����
	for (int row = 0; row < row_length; row++) {
		for (int column = 0; column < column_length - 1; column++) {
			int pos_temp = row*column_length + column;
			int pos = row*S0 + column*S1;
			int pos_next = pos + S1;
			if (image[pos] > image[pos_next]) {
				difference_integral[pos_temp] = image[pos] - image[pos_next];
			}
			else {
				difference_integral[pos_temp] = image[pos_next] - image[pos];
			}
			//���л���
			if (column > 0) {
				difference_integral[pos_temp] += difference_integral[pos_temp - 1];
			}
		}
	}
	for (int row = 0; row < row_length; row++) {
		for (int column = 0; column < column_length - window_size; column++) {
			int pos_difference = row*column_length + column;
			int pos = row*S0 + (column + window_size / 2)*S1;
			int sum = 0;
			
			sum = difference_integral[pos_difference + window_size-1];
			if (column > 0) {
				sum -= difference_integral[pos_difference - 1];
			}
			if (sum < texture_range) {
				//��ȡ���� ����ͳ����������������
				int low_count = 1;
				if (column + window_size / 2>0) {
					low_count += column_result[pos - 1];
				}
				column_result[pos] = low_count;
				//��֮ǰ�����ݽ��и���
				for (int i = pos-1; i >= row*S0; i--) {
					if (column_result[i]) {
						column_result[i] = low_count;
					}
					else {
						break;
					}
				}
				//��Ե��������
				if (column == 0) {
					for (int i = 0; i <= window_size / 2; i++) {
						column_result[pos-i] = low_count;
					}
				}
				else if (column == column_length - window_size - 1) {
					for (int i = 0; i <= window_size / 2; i++) {
						column_result[pos+i] = low_count;
					}
				}
			}
			else {
				column_result[pos] = 0;
			}
		}
	}
	delete(difference_integral);
	//�õ��������д���
	for (int column = 0; column < column_length - window_size; column++) {
		for (int row = 0; row < row_length; row++) {
			int pos = row*S0 + column*S1;
			if (column_result[pos]) {
				int low_count = 1;
				if (row > 0) {
					low_count += row_result[pos - row*S0];
				}
				row_result[pos] = low_count;
				//��֮ǰ�����ݽ��и���
				for (int i = pos - S0; i >= 0; i -= S0) {
					if (row_result[i]) {
						row_result[i] = low_count;
					}
					else {
						break;
					}
				}
				
			}
		}
	}
}
//���۾ۺ� ʹ�õ������������õĴ���
void __stdcall aggregate_cost_window(INT32 result[], INT16 diff[], const INT32 diff_strides[], const INT32 result_strides[], const INT16 shapes[], const INT16 row_window[], const INT16 column_window[], const INT16 window_size_min, const INT16 window_size_max) {
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
		//�õ�����ͼ
		INT32* integral_result = new INT32[row_length*column_length];
		integral(integral_result, diff_this_deep, row_length, column_length);
		for (int row = 0; row < row_length; row++) {
			for (int column = 0; column < column_length; column++) {
				//�д��ڴ�С
				int pos = row*column_length + column;
				int row_window_size = row_window[pos];
				int column_window_size = column_window[pos];
				//�߽���
				if (row_window_size < window_size_min) {
					row_window_size = window_size_min;
				}
				else if (row_window_size > window_size_max) {
					row_window_size = window_size_max;
				}
				if (column_window_size < window_size_min) {
					column_window_size = window_size_min;
				}
				else if (column_window_size > window_size_max) {
					column_window_size = window_size_max;
				}

				//��ȷ���쳣�߽�
				int top = 0, bottom = row_length;
				//������� ����ȷ���߽�
				if (row - row_window_size / 2 > 0) {
					top = row - row_window_size / 2;
				}
				if (row + row_window_size / 2 + 1 <= row_length) {
					bottom = row + row_window_size / 2 + 1;
				}
				//��ȷ���쳣�߽�
				int left = 0, right = column_length;
				//������� ����ȷ���߽�
				if (column - column_window_size / 2 > 0) {
					left = column - column_window_size / 2;
				}
				if (column + column_window_size / 2 + 1 <= column_length) {
					right = column + column_window_size / 2 + 1;
				}

				//��� SAD
				INT32 sad = integral_result[(bottom - 1)*column_length + (right - 1)];

				if (left >0) {
					sad -= integral_result[(bottom - 1)*column_length + (left - 1)];
				}
				if (top > 0 && left >0) {
					sad += integral_result[(top - 1)*column_length + (left - 1)];
				}
				if (top > 0) {
					sad -= integral_result[(top - 1)*column_length + (right - 1)];
				}
				//��һ��
				INT32 sad_normal = 100 * sad / (bottom - top) / (right - left);
				result[row*S_row_r + column*S_column_r + d*S_deep_r] = sad_normal;
			}
		}
		delete(integral_result);
	}
}

__inline void  get_low_texture_cost_core(UINT8& last_count, bool& last_texture, UINT8 result[], INT16 low_texture[],int pos) {
	int value = 0;
	if (low_texture[pos] > 5) {
		if (last_texture) {
			last_count++;
			value = last_count;
		}
		else {
			last_texture = true;
		}
	}
	else {
		last_texture = false;
		last_count = 0;
	}
}


//Ϊ����������Ƶ����ƥ���㷨
void __stdcall get_low_texture_cost_l(INT16 result[], INT16 low_texture[], const INT32 shapes[]) {
	//row size
	int row_length = shapes[0];
	//column size
	int column_length = shapes[1];
	for (int row = 0; row < row_length; row++) {
		int last_count = 0;
		bool last_texture = false;
		//Ϊ��ֹһ��ʼ���ǵ���������ɴ���
		int start_pos = 5;
		//�����ǵ�������
		while (start_pos < column_length) {
			int pos = row*column_length + start_pos;
			if (low_texture[start_pos] > 0) {
				start_pos++;
			}
			else {
				break;
			}
		}
		for (int column = start_pos; column < column_length; column++) {
			int pos = row*column_length + column;
			int value = 0;
			if (low_texture[pos] > 0) {
				if (last_texture) {
					last_count++;
					value = last_count;
				}
				else {
					last_texture = true;
				}
			}
			else {
				last_texture = false;
				last_count = 0;
			}
			result[pos] = value;
		}
	}

}

//Ϊ����������Ƶ��Ҳ�ƥ���㷨
void __stdcall get_low_texture_cost_r(INT16 result[], INT16 low_texture[], const INT32 shapes[]) {
	//row size
	int row_length = shapes[0];
	//column size
	int column_length = shapes[1];
	for (int row = 0; row < row_length; row++) {
		UINT8 last_count = 0;
		bool last_texture = false;
		for (int column = column_length-1; column >= 0; column--) {
			int pos = row*column_length + column;
			int value = 0;
			if (low_texture[pos] > 0) {
				if (last_texture) {
					last_count++;
					value = last_count;
				}
				else {
					last_texture = true;
				}
			}
			else {
				last_texture = false;
				last_count = 0;
			}
			result[pos] = value;
		}
	}

}