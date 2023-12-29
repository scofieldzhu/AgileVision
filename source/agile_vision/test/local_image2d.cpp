/* 
 *   AgileVison is a generic vision framework, which provides some functional modules
 *   to make you more easier to fast construct your project vison solution implementation.
 *  
 *   File: local_image2d.cpp  
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

#include "local_image2d.h"
#include "ratel/basic/dbg_tracker.h"
#include "spdlog/spdlog.h"
using namespace agile_vision;
using namespace spdlog;
using namespace cv;

LocalImage2d::LocalImage2d(const std::string& iid)
    :Tool(iid)
{
    addPin(PK_O_ImageData, std::make_shared<OutputPin>(DataSpec::SingleImage()));
    addPin(PK_P_DirType, std::make_shared<PropPin>(DataSpec::SingleInt()));
    getPropPin(PK_P_DirType)->mutableDataBuffer().setIntValue(0);
    addPin(PK_P_ImagePath, std::make_shared<PropPin>(DataSpec::SingleString()));
}

LocalImage2d::~LocalImage2d()
{
}

std::string LocalImage2d::getClsGuid() const
{
    return "861d31cd-d3f1-42d0-a8a4-8c618445891a";
}

bool LocalImage2d::requestOutputData()
{
    _AUTO_FUNC_TRACK_
    auto dir_type = getPropPin(PK_P_DirType)->dataBuffer().getIntValue();
    auto image_path = getPropPin(PK_P_ImagePath)->dataBuffer().getStringValue();
    if(dir_type == 0){ // regular file
        image_ = imread(image_path, cv::IMREAD_COLOR);
        //cv::imshow("Display window", image_);
        if(image_.empty()){
            spdlog::error("Image path:'{}' not exists!", image_path);
            return false;
        }
    }else{
        spdlog::error("Image dir path type not supported at present!", image_path);
        return false;
    }
    ImageData img;
    if(image_.type() == CV_8U){
        img.type = ImageData::DT_UCHAR;
    }else if(image_.type() == CV_16U){
        img.type = ImageData::DT_USHORT;
    }else if(image_.type() == CV_32S){
        img.type = ImageData::DT_INT;
    }else if(image_.type() == CV_32F){
        img.type = ImageData::DT_FLOAT;
    }
    img.bits = (uint8_t)(image_.elemSize1() * 8);
    img.width = image_.cols;
    img.height = image_.rows;
    img.size = (uint32_t)(image_.total() * image_.elemSize());
    img.data = image_.ptr(0);
    getOutputPin(PK_O_ImageData)->mutableDataBuffer().setImageValue(img);
    spdlog::trace("Read image data from path:{} successfully!", image_path);
    return true;
}
