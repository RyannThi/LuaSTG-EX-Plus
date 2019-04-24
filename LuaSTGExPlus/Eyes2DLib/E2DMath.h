#pragma once
/*========================================*\
|*Code by Xiliusha                        *|
|*一些辅助类型的数学函数                  *|
\*========================================*/

#include <cmath>

namespace Eyes2D {
	namespace Math {
		//分贝转电平，电平以1V为基准
		inline double dBConvertToValue(double dB) {
			return std::pow(10.0, dB / 20.0);
		}

		//电平转分贝，电平以1V为基准
		inline double ValueConvertTodB(double v) {
			return 20 * std::log10(v);
		}

		//音量的线性转对数
		//<param[in] v double --取值范围{0~1}，归一化线性音量
		//>return[out] double --值范围{0~1}，归一化分贝衰减模型对数音量
		double LinearToLog(double v);

		//音量的对数转线性
		//<param[in] d double --取值范围{0~1}，归一化分贝衰减模型对数音量
		//>return[out] double --值范围{0~1}，归一化线性音量
		double LogToLinear(double d);
	}
}
