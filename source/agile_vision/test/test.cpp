/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: test.cpp  
 *   Copyright (c) 2023-2023 scofieldzhu
 *  
 *   MIT License
 *  
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *  
 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 *  
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#include "test_solution.h"
#include "spdlog/spdlog.h"

int main()
{
    spdlog::set_level(spdlog::level::trace);
    spdlog::trace("Enter test main...");

    Test_Solution();

    spdlog::trace("Leave test main...");
    return 0;
}

// #include <opencv2/opencv.hpp> // 包含OpenCV主要头文件

// int main() {
//     // 图片路径，替换为你的图片路径
//     std::string image_path = "F:\\Program Files\\CKVisionBuilder\\Images\\ImageCalib.bmp";

//     // 读取图片
//     cv::Mat img = cv::imread(image_path, cv::IMREAD_COLOR);

//     // 判断图片是否成功加载
//     if(img.empty()) {
//         std::cerr << "Could not read the image: " << image_path << std::endl;
//         return 1;
//     }

//     // 显示图片
//     cv::imshow("Display window", img);

//     // 等待用户输入，参数为0表示无限期等待
//     cv::waitKey(0); 

//     return 0;
// }
