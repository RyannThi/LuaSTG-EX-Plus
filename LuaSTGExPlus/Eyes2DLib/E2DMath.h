#pragma once
/*========================================*\
|*Code by Xiliusha                        *|
|*һЩ�������͵���ѧ����                  *|
\*========================================*/

#include <cmath>

namespace Eyes2D {
	namespace Math {
		//�ֱ�ת��ƽ����ƽ��1VΪ��׼
		inline double dBConvertToValue(double dB) {
			return std::pow(10.0, dB / 20.0);
		}

		//��ƽת�ֱ�����ƽ��1VΪ��׼
		inline double ValueConvertTodB(double v) {
			return 20 * std::log10(v);
		}

		//����������ת����
		//<param[in] v double --ȡֵ��Χ{0~1}����һ����������
		//>return[out] double --ֵ��Χ{0~1}����һ���ֱ�˥��ģ�Ͷ�������
		double LinearToLog(double v);

		//�����Ķ���ת����
		//<param[in] d double --ȡֵ��Χ{0~1}����һ���ֱ�˥��ģ�Ͷ�������
		//>return[out] double --ֵ��Χ{0~1}����һ����������
		double LogToLinear(double d);
	}
}
