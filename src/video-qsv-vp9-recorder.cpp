/*
 * Copyright (C) 2019  Christian Berger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cluon-complete.hpp"
#include "opendlv-standard-message-set.hpp"

#include <YamiC.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>

// docker run --rm -ti --init --device /dev/dri/renderD128 -v /usr/lib/x86_64-linux-gnu/dri:/usr/lib/x86_64-linux-gnu/dri -v $PWD:/data qsv
// ./video-qsv-vp9-recorder /data/in.yuv --cid=111 --name=data --width=640 --height=480

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{1};
    const uint32_t ZERO{0};
    const uint32_t ONE{1};
    const uint32_t FOUR{4};
    const uint32_t FIFTYONE{51};

    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("name")) ||
         (0 == commandlineArguments.count("width")) ||
         (0 == commandlineArguments.count("height")) ||
         (0 == commandlineArguments.count("cid")) ) {
        std::cerr << argv[0] << " attaches to an I420-formatted image residing in a shared memory area to convert it into a corresponding h264 frame for publishing to a running OD4 session using Intel QuickSync; supports cropping, flipping, and scaling" << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --cid=<OpenDaVINCI session> --name=<name of shared memory area> --width=<width> --height=<height> [--gop=<GOP>] [--verbose] [--id=<identifier in case of multiple instances] [--bitrate=<bitrate>] [--ip-period=<ip-period>] "
                "[--init-qp=<init-qp>] [--qpmin=<qpmin>] [--qpmax=<qpmax>] [--disable-frame-skip=<disable-frame-skip>] [--diff-qp-ip=<diff-qp-ip>] [--diff-qp-ib=<diff-qp-ib>] [--num-ref-frame=<num-ref-frame>] [--rc-mode=<rc-mode>] [--flip] "
                "[--rc-mode=<rc-mode>] [--reference-mode=<reference-mode>]"<< std::endl;
        std::cerr << "         --cid:             CID of the OD4Session to listen for Envelopes to record (also needed for remote control)" << std::endl;
        std::cerr << "         --id:              when using several instances, this identifier is used as senderStamp" << std::endl;
        std::cerr << "         --name:            name of the shared memory area to attach" << std::endl;
        std::cerr << "         --rec:             name of the recording file; default: YYYY-MM-DD_HHMMSS.rec" << std::endl;
        std::cerr << "         --recsuffix:       additional suffix to add to the .rec file" << std::endl;
        std::cerr << "         --remote:          enable remote control for start/stop recording" << std::endl;
        std::cerr << "         --width:           width of the frame" << std::endl;
        std::cerr << "         --height:          height of the frame" << std::endl;
        std::cerr << "         --gop:             optional: length of group of pictures (default = 1)" << std::endl;
        std::cerr << "         --bitrate:         optional: (default = 8000)" << std::endl;
        std::cerr << "         --ip-period:       optional: 0 (I frame only) | 1 (I and P frames) | N (I,P and B frames, B frame number is N-1) (default = 1)" << std::endl;
        std::cerr << "         --init-qp          optional: initial QP (default: 23)" << std::endl;
        std::cerr << "         --qpmin            optional: minimum quantizer that x264 will ever use (default: 0)" << std::endl;
        std::cerr << "         --qpmax            optional: maximum quantizer that x264 will ever use (default: 51)" << std::endl;
        std::cerr << "         --disable-frame-skip: optional: toggle frame-skipping (default: 0)" << std::endl;
        std::cerr << "         --diff-qp-ip:      optional: QP difference between adjacent I/P (default: 0)" << std::endl;
        std::cerr << "         --diff-qp-ib:      optional: QP difference between adjacent I/B (default: 0)" << std::endl;
        std::cerr << "         --num-ref-frame:   optional: number of reference frame used (default: 1)" << std::endl;
        std::cerr << "         --rc-mode:         optional: rate control mode (default: 4, 0: NONE, 1: CBR, 2: VBR, 3: VCM, 4: CQP)" << std::endl;
        std::cerr << "         --reference-mode:  optional: reference frames mode (default: 0, 0: last(previous) gold/alt (previous key frame), 1: last (previous) gold (one before last) alt (one before gold))" << std::endl;
        std::cerr << "         --verbose:         print encoding information" << std::endl;
        std::cerr << "Example: " << argv[0] << " --cid=111 --name=video0.i420 --width=640 --height=480 --verbose" << std::endl;
    }
    else {
        auto getYYYYMMDD_HHMMSS = [](){
            cluon::data::TimeStamp now = cluon::time::now();

            const long int _seconds = now.seconds();
            struct tm *tm = localtime(&_seconds);

            uint32_t year = (1900 + tm->tm_year);
            uint32_t month = (1 + tm->tm_mon);
            uint32_t dayOfMonth = tm->tm_mday;
            uint32_t hours = tm->tm_hour;
            uint32_t minutes = tm->tm_min;
            uint32_t seconds = tm->tm_sec;

            std::stringstream sstr;
            sstr << year << "-" << ( (month < 10) ? "0" : "" ) << month << "-" << ( (dayOfMonth < 10) ? "0" : "" ) << dayOfMonth
                           << "_" << ( (hours < 10) ? "0" : "" ) << hours
                           << ( (minutes < 10) ? "0" : "" ) << minutes
                           << ( (seconds < 10) ? "0" : "" ) << seconds;

            std::string retVal{sstr.str()};
            return retVal;
        };

        const bool REMOTE{commandlineArguments.count("remote") != 0};
        const std::string RECSUFFIX{commandlineArguments["recsuffix"]};
        const std::string REC{(commandlineArguments["rec"].size() != 0) ? commandlineArguments["rec"] : ""};
        const std::string NAME_RECFILE{(REC.size() != 0) ? REC + RECSUFFIX : (getYYYYMMDD_HHMMSS() + RECSUFFIX + ".rec")};

        const std::string NAME{commandlineArguments["name"]};
        const uint32_t WIDTH{static_cast<uint32_t>(std::stoi(commandlineArguments["width"]))};
        const uint32_t HEIGHT{static_cast<uint32_t>(std::stoi(commandlineArguments["height"]))};
        const uint32_t GOP_DEFAULT{1};
        const uint32_t GOP{(commandlineArguments["gop"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["gop"])) : GOP_DEFAULT};
        const bool VERBOSE{commandlineArguments.count("verbose") != 0};
        const uint32_t CID{(commandlineArguments["cid"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["cid"])) : 0};
        const uint32_t ID{(commandlineArguments["id"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["id"])) : 0};
        const uint32_t FPS_DEFAULT{30};
        const uint32_t BITRATE_DEFAULT{8000};
        const uint32_t BITRATE{((commandlineArguments["bitrate"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["bitrate"])) : BITRATE_DEFAULT) * 1024};

        const uint32_t IP_PERIOD{(commandlineArguments["ip-period"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["ip-period"])) : 1};
        const uint32_t INIT_QP{(commandlineArguments["init-qp"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["init-qp"])) : 26};
        const uint32_t QPMIN{(commandlineArguments["qpmin"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["qpmin"])), ZERO), FIFTYONE): 0};
        const uint32_t QPMAX{(commandlineArguments["qpmax"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["qpmax"])), ZERO), FIFTYONE): 51};
        const uint32_t FRAMESKIP{(commandlineArguments["disable-frame-skip"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["disable-frame-skip"])), ZERO), ONE): 1};
        const int8_t DIFF_QP_IP{(commandlineArguments["diff-qp-ip"].size() != 0) ? static_cast<int8_t>(std::stoi(commandlineArguments["diff-qp-ip"])) : 0};
        const int8_t DIFF_QP_IB{(commandlineArguments["diff-qp-ib"].size() != 0) ? static_cast<int8_t>(std::stoi(commandlineArguments["diff-qp-ib"])) : 0};
        const uint32_t NUM_REF_FRAME{(commandlineArguments["num-ref-frame"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["num-ref-frame"])) : 1};
        const uint32_t RC_MODE{(commandlineArguments["rc-mode"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["rc-mode"])), ZERO), FOUR): 4};

        const uint32_t REFERENCE_MODE{(commandlineArguments["reference-mode"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["reference-mode"])), ZERO), ONE): 0};

        std::unique_ptr<cluon::SharedMemory> sharedMemory(new cluon::SharedMemory{NAME});
        if (sharedMemory && sharedMemory->valid()) {
            std::clog << "[video-qsv-vp9-recorder]: Attached to '" << sharedMemory->name() << "' (" << sharedMemory->size() << " bytes)." << std::endl;

            std::unique_ptr<cluon::OD4Session> od4Session{nullptr};
            std::mutex recFileMutex{};
            std::unique_ptr<std::fstream> recFile{nullptr};
            std::string nameOfRecFile;
            if (!REMOTE) {
                recFile.reset(new std::fstream(NAME_RECFILE.c_str(), std::ios::out|std::ios::binary|std::ios::trunc));
                std::clog << "[video-qsv-vp9-recorder]: Created " << NAME_RECFILE << "." << std::endl;
            }
            else {
                if (CID == 0) {
                    std::cerr << "[video-qsv-vp9-recorder]: --remote specified but no --cid=? provided." << std::endl;
                    return retCode;
                }

                od4Session.reset(new cluon::OD4Session(CID,
                    [REC, RECSUFFIX, getYYYYMMDD_HHMMSS, &recFileMutex, &recFile, &nameOfRecFile](cluon::data::Envelope &&envelope) noexcept {
                    if (cluon::data::RecorderCommand::ID() == envelope.dataType()) {
                        std::lock_guard<std::mutex> lck(recFileMutex);
                        cluon::data::RecorderCommand rc = cluon::extractMessage<cluon::data::RecorderCommand>(std::move(envelope));
                        if (1 == rc.command()) {
                            if (recFile && recFile->good()) {
                                recFile->flush();
                                recFile->close();
                                recFile = nullptr;
                                std::clog << "[video-qsv-vp9-recorder]: Closed " << nameOfRecFile << "." << std::endl;
                            }
                            nameOfRecFile = (REC.size() != 0) ? REC + RECSUFFIX : (getYYYYMMDD_HHMMSS() + RECSUFFIX + ".rec");
                            recFile.reset(new std::fstream(nameOfRecFile.c_str(), std::ios::out|std::ios::binary|std::ios::trunc));
                            std::clog << "[video-qsv-vp9-recorder]: Created " << nameOfRecFile << "." << std::endl;
                        }
                        else if (2 == rc.command()) {
                            if (recFile && recFile->good()) {
                                recFile->flush();
                                recFile->close();
                                std::clog << "[video-qsv-vp9-recorder]: Closed " << nameOfRecFile << "." << std::endl;
                            }
                            recFile = nullptr;
                        }
                    }
                }));
            }

            EncodeHandler encodeHandler = createEncoder(YAMI_MIME_VP9);
            if (nullptr != encodeHandler) {
                YamiStatus retVal{YAMI_SUCCESS};
                {
                    VideoParamsCommon encVideoParams;
                    encVideoParams.size = sizeof(VideoParamsCommon);
                    retVal = encodeGetParameters(encodeHandler, VideoParamsTypeCommon, &encVideoParams);
                    if (YAMI_SUCCESS != retVal) {
                        std::cerr << "[video-qsv-vp9-recorder]: Error retrieving parameters 'VideoParamsTypeCommon': " << retVal << std::endl;
                    }

                    {
                        // https://github.com/intel/libyami/blob/apache/interface/VideoEncoderDefs.h
                        // https://github.com/intel/libyami-utils/blob/master/doc/yamitranscode.1
                        encVideoParams.resolution.width = WIDTH;
                        encVideoParams.resolution.height = HEIGHT;

                        encVideoParams.frameRate.frameRateDenom = 1;
                        encVideoParams.frameRate.frameRateNum = FPS_DEFAULT;

                        encVideoParams.intraPeriod = GOP;
                        encVideoParams.ipPeriod = IP_PERIOD;

                        encVideoParams.rcParams.bitRate = BITRATE;
                        encVideoParams.rcParams.initQP = INIT_QP; // Initial quality factor.
                        encVideoParams.rcParams.minQP = QPMIN;
                        encVideoParams.rcParams.maxQP = QPMAX;
                        encVideoParams.rcParams.disableFrameSkip = FRAMESKIP;
                        encVideoParams.rcParams.diffQPIP = DIFF_QP_IP;
                        encVideoParams.rcParams.diffQPIB = DIFF_QP_IB;

                        encVideoParams.numRefFrames = NUM_REF_FRAME;
                        encVideoParams.enableLowPower = false;
                        encVideoParams.bitDepth = 8;

                        switch (RC_MODE) {
                            case 0: { encVideoParams.rcMode = RATE_CONTROL_NONE; break; }
                            case 1: { encVideoParams.rcMode = RATE_CONTROL_CBR; break; }
                            case 2: { encVideoParams.rcMode = RATE_CONTROL_VBR; break; }
                            case 3: { encVideoParams.rcMode = RATE_CONTROL_VCM; break; }
                            case 4: { encVideoParams.rcMode = RATE_CONTROL_CQP; break; }
                        }
                        

                        encVideoParams.size = sizeof(VideoParamsCommon);
                    }
                    retVal = encodeSetParameters(encodeHandler, VideoParamsTypeCommon, &encVideoParams);
                    if (YAMI_SUCCESS != retVal) {
                        std::cerr << "[video-qsv-vp9-recorder]: Error setting parameters 'VideoParamsTypeCommon': " << retVal << std::endl;
                    }
                }

                {
                    VideoParamsVP9 encVideoParams;
                    retVal = encodeGetParameters(encodeHandler, VideoParamsTypeVP9, &encVideoParams);
                    if (YAMI_SUCCESS != retVal) {
                        std::cerr << "[video-qsv-vp9-recorder]: Error retrieving parameters 'VideoParamsTypeVP9': " << retVal << std::endl;
                    }

                    {
                        encVideoParams.referenceMode = REFERENCE_MODE;
                    }
                    retVal = encodeSetParameters(encodeHandler, VideoParamsTypeVP9, &encVideoParams);
                    if (YAMI_SUCCESS != retVal) {
                        std::cerr << "[video-qsv-vp9-recorder]: Error setting parameters 'VideoParamsTypeVP9': " << retVal << std::endl;
                    }
                }

                retVal = encodeStart(encodeHandler);
                if (YAMI_SUCCESS != retVal) {
                    std::cerr << "[video-qsv-vp9-recorder]: Error starting encoder: " << retVal << std::endl;
                }

                VideoFrameRawData inBuffer;
                {
                    inBuffer.memoryType = VIDEO_DATA_MEMORY_TYPE_RAW_POINTER;
                    inBuffer.size = WIDTH * HEIGHT * 3/2; // I420 is W*H*3/2
                    inBuffer.handle = reinterpret_cast<intptr_t>(reinterpret_cast<uint8_t*>(sharedMemory->data()));
                    inBuffer.width = WIDTH;
                    inBuffer.height = WIDTH;
                    inBuffer.pitch[0] = WIDTH;   // Y
                    inBuffer.pitch[1] = WIDTH/2; // U
                    inBuffer.pitch[2] = WIDTH/2; // V
                    inBuffer.offset[0] = 0; // Y
                    inBuffer.offset[1] = WIDTH*HEIGHT; // U
                    inBuffer.offset[2] = inBuffer.offset[1] + (WIDTH*HEIGHT)/4; // V
                    inBuffer.fourcc = YAMI_FOURCC_I420;
                    inBuffer.internalID = 0;
                    inBuffer.timeStamp = 0;
                    inBuffer.flags = VIDEO_FRAME_FLAGS_KEY;
                }

                // Reserve RGB-sized buffer per frame that should be large enough to hold a lossy-encoded h264 frame.
                const uint32_t INTERNAL_BUFFER_SIZE{WIDTH * HEIGHT * 3};
                uint8_t internalBuffer[INTERNAL_BUFFER_SIZE];
                VideoEncOutputBuffer outBuffer;
                {
                    outBuffer.data = internalBuffer;
                    outBuffer.bufferSize = INTERNAL_BUFFER_SIZE;
                    outBuffer.dataSize = 0;
                    outBuffer.remainingSize = 0;
                    outBuffer.flag = 0;
                    outBuffer.format = OUTPUT_EVERYTHING;
                    outBuffer.temporalID = 0;
                    outBuffer.timeStamp = 0;
                }

                cluon::data::TimeStamp before, after, sampleTimeStamp;

                while ( (YAMI_SUCCESS == retVal) &&
                        (sharedMemory && sharedMemory->valid()) &&
                        !cluon::TerminateHandler::instance().isTerminated.load() ) {
                    // Wait for incoming frame.
                    sharedMemory->wait();

                    sampleTimeStamp = cluon::time::now();

                    sharedMemory->lock();
                    {
                        // Read notification timestamp.
                        auto r = sharedMemory->getTimeStamp();
                        sampleTimeStamp = (r.first ? r.second : sampleTimeStamp);

                        {
                            if (VERBOSE) {
                                before = cluon::time::now();
                            }

                            retVal = encodeEncodeRawData(encodeHandler, &inBuffer);
                            if (YAMI_SUCCESS != retVal) {
                                std::cerr << "[video-qsv-vp9-recorder]: Error encoding frame: " << retVal << std::endl;
                            }

                            if (VERBOSE) {
                                after = cluon::time::now();
                            }
                        }

                        if (YAMI_SUCCESS != retVal) {
                            sharedMemory->unlock();
                            break;
                        }

                        bool withWait = true;
                        retVal = encodeGetOutput(encodeHandler, &outBuffer, withWait);
                        if (YAMI_SUCCESS == retVal) {
                            if ( (0 < outBuffer.dataSize) && (recFile && recFile->good()) ) {
                                std::string data(reinterpret_cast<char*>(internalBuffer), outBuffer.dataSize);

                                opendlv::proxy::ImageReading ir;
                                ir.fourcc("VP90").width(WIDTH).height(HEIGHT).data(data);
                                {
                                    cluon::data::Envelope envelope;
                                    {
                                        cluon::ToProtoVisitor protoEncoder;
                                        {
                                            envelope.dataType(ir.ID());
                                            ir.accept(protoEncoder);
                                            envelope.serializedData(protoEncoder.encodedData());
                                            envelope.sent(cluon::time::now());
                                            envelope.sampleTimeStamp(sampleTimeStamp);
                                            envelope.senderStamp(ID);
                                        }
                                    }

                                    std::lock_guard<std::mutex> lck(recFileMutex);
                                    std::string serializedData{cluon::serializeEnvelope(std::move(envelope))};
                                    recFile->write(serializedData.data(), serializedData.size());
                                    recFile->flush();
                                }

                                if (VERBOSE) {
                                    std::clog << "[video-qsv-vp9-recorder]: Frame size = " << data.size() << " bytes; sample time = " << cluon::time::toMicroseconds(sampleTimeStamp) << " microseconds; encoding took " << cluon::time::deltaInMicroseconds(after, before) << " microseconds." << std::endl;
                                }
                            }
                        }
                        else {
                            std::cerr << "[video-qsv-vp9-recorder]: Error getting encoded frame: " << retVal << std::endl;
                            sharedMemory->unlock();
                            break;
                        }
                    }
                    sharedMemory->unlock();
                }

                encodeStop(encodeHandler);
                releaseEncoder(encodeHandler);
            }
            else {
                std::cerr << "[video-qsv-vp9-recorder]: Error creating encoding handler." << std::endl;
            }

            retCode = 0;
        }
        else {
            std::cerr << "[video-qsv-vp9-recorder]: Failed to attach to shared memory '" << NAME << "'." << std::endl;
        }
    }
    return retCode;
}
