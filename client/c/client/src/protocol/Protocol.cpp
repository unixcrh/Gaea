/*
 *  Copyright Beijing 58 Information Technology Co.,Ltd.
 *
 *  Licensed to the Apache Software Foundation (ASF) under one
 *  or more contributor license agreements.  See the NOTICE file
 *  distributed with this work for additional information
 *  regarding copyright ownership.  The ASF licenses this file
 *  to you under the Apache License, Version 2.0 (the
 *  "License"); you may not use this file except in compliance
 *  with the License.  You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an
 *  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 *  specific language governing permissions and limitations
 *  under the License.
 */
/*
 * Protocol.cpp
   *
 * Created on: 2011-7-7
 * Author: Service Platform Architecture Team (spat@58.com)
 *
 * 版本一协义定义
 *
 * 1byte(版本号) | 4byte(协义总长度) | 4byte(序列号) | 1byte(服务编号) | 1byte(消息体类型) | 1byte 所采用的压缩算法 | 1byte 序列化规则 | 1byte 平台(.net java ...) | n byte消息体 | 5byte(分界符)
 *    0                1~4              5~8              9                   10                   11                      12                     13
 * 消息头总长度:14byte
 *
 * 协义总长度 = 消息头总长度 + 消息体总长度 (不包括分界符)
 *
 * 尾分界符: 9, 11, 13, 17, 18
 *
 * 版本号从ASCII > 48 开始标识
 *
 */

#include "Protocol.h"
#include "../serialize/serializer.h"
#include "../serialize/derializer.h"
#include "SFPStruct.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdexcept>
#include <string>
namespace gaea {
Protocol::Protocol(int sessionId, char serviceId, SDPType sdpType, CompressType compressType, SerializeType serializeType, PlatformType platformType, char* userData) {
	this->sessionId = sessionId;
	this->serviceId = serviceId;
	this->sdpType = sdpType;
	this->compressType = compressType;
	this->serializeType = serializeType;
	this->platformType = platformType;
	this->userData = userData;
}
Protocol::Protocol(int sessionId, char serviceId, SDPType sdpType, CompressType compressType, SerializeType serializeType, PlatformType platformType, void *sdpEntity,
		const char *sdpEntityType) {
	this->sessionId = sessionId;
	this->serviceId = serviceId;
	this->sdpType = sdpType;
	this->compressType = compressType;
	this->serializeType = serializeType;
	this->platformType = platformType;
	this->sdpEntity = sdpEntity;
	this->sdpEntityType = (char*) sdpEntityType;
	this->userData = NULL;
}
Protocol::Protocol(int sessionId, char serviceId, SDPType sdpType, void* sdpEntity, const char *sdpEntityType) {
	this->sessionId = sessionId;
	this->serviceId = serviceId;
	this->sdpType = sdpType;
	this->sdpEntity = sdpEntity;
	this->sdpEntityType = (char*) sdpEntityType;
	this->userData = NULL;
	this->serializeType = GAEABinary;
	this->compressType = UnCompress;
	this->platformType = C;
}
Protocol::Protocol() {

}
Protocol::~Protocol() {
}
char* Protocol::getBytes(int &dataLen) {
	int startIndex = 0;
	int serDataLen = 0;
	char* sdpData = Serialize(sdpEntityType, sdpEntity, &serDataLen);
	int protocolLen = HEAD_STACK_LENGTH + serDataLen;

	char *data = new char[protocolLen];
	data[0] = Protocol::VERSION;

	startIndex += SFPStruct::Version;
	memcpy(data + startIndex, &protocolLen, SFPStruct::TotalLen);

	startIndex += SFPStruct::TotalLen;
	memcpy(data + startIndex, &sessionId, SFPStruct::SessionId);

	startIndex += SFPStruct::SessionId;
	data[startIndex] = serviceId;

	startIndex += SFPStruct::ServerId;
	data[startIndex] = sdpType;

	startIndex += SFPStruct::SDPType;
	data[startIndex] = compressType;

	startIndex += SFPStruct::CompressType;
	data[startIndex] = serializeType;

	startIndex += SFPStruct::SerializeType;
	data[startIndex] = platformType;

	startIndex += SFPStruct::Platform;
	memcpy(data + startIndex, sdpData, serDataLen);

	free(sdpData);
	serData = data;
	dataLen = protocolLen;
	return data;
}
int Protocol::getSessionID() {
	return sessionId;
}
Protocol* Protocol::fromBytes(const char* data, int dataLen) {
	if (dataLen == 0 || data == NULL) {
		return NULL;
	}
	for (int i = 0; i < dataLen; ++i) {
		printf("%02x ", data[i]);
	}
	printf("\n");

	Protocol *p = new Protocol();
	int startIndex = 0;
	if (data[startIndex] != Protocol::VERSION) {
		errno = -2;
		throw std::runtime_error("协义版本错误");
	}

	startIndex += SFPStruct::Version;
	char totalLengthByte[SFPStruct::TotalLen];
	for (int i = 0; i < SFPStruct::TotalLen; i++) {
		totalLengthByte[i] = data[startIndex + i];
	}
	p->setTotalLen(*(int*) totalLengthByte);

	startIndex += SFPStruct::TotalLen;
	char sessionIDByte[SFPStruct::SessionId];
	for (int i = 0; i < SFPStruct::SessionId; i++) {
		sessionIDByte[i] = data[startIndex + i];
	}
	p->setSessionId(*(int*) sessionIDByte);

	startIndex += SFPStruct::SessionId;
	p->setServiceId(data[startIndex]);

	startIndex += SFPStruct::ServerId;
	p->setSdpType((SDPType) data[startIndex]);

	startIndex += SFPStruct::SDPType;
	p->setCompressType((CompressType) data[startIndex]);

	startIndex += SFPStruct::CompressType;
	p->setSerializeType((SerializeType) data[startIndex]);

	startIndex += SFPStruct::SerializeType;
	p->setPlatformType((PlatformType) data[startIndex]);

	startIndex += SFPStruct::Platform;

	p->setSdpEntity(Derialize((char*) getSDPClass(p->getSdpType()), (void*) (data + startIndex), dataLen - startIndex));
	return p;
}
const char* Protocol::getSDPClass(SDPType type) {
	std::string s;
	if (type == Request) {
		return "RequestProtocol";
	} else if (type == Response) {
		return "ResponseProtocol";
	} else if (type == Exception) {
		return "ExceptionProtocol";
	} else if (type == Handclasp) {
		return "HandclaspProtocol";
	} else if (type == RebootException) {
		return "ResetProtocol";
	}
	errno = -2;
	throw std::runtime_error("末知的SDP类型");
}
CompressType Protocol::getCompressType() const {
	return compressType;
}

PlatformType Protocol::getPlatformType() const {
	return platformType;
}

void *Protocol::getSdpEntity() const {
	return sdpEntity;
}

char *Protocol::getSdpEntityType() const {
	return sdpEntityType;
}

SDPType Protocol::getSdpType() const {
	return sdpType;
}

SerializeType Protocol::getSerializeType() const {
	return serializeType;
}

char Protocol::getServiceId() const {
	return serviceId;
}

int Protocol::getSessionId() const {
	return sessionId;
}

int Protocol::getTotalLen() const {
	return totalLen;
}

char *Protocol::getUserData() const {
	return userData;
}

void Protocol::setCompressType(CompressType compressType) {
	this->compressType = compressType;
}

void Protocol::setPlatformType(PlatformType platformType) {
	this->platformType = platformType;
}

void Protocol::setSdpEntity(void *sdpEntity) {
	this->sdpEntity = sdpEntity;
}

void Protocol::setSdpEntityType(char *sdpEntityType) {
	this->sdpEntityType = sdpEntityType;
}

void Protocol::setSdpType(SDPType sdpType) {
	this->sdpType = sdpType;
}

void Protocol::setSerializeType(SerializeType serializeType) {
	this->serializeType = serializeType;
}

void Protocol::setServiceId(char serviceId) {
	this->serviceId = serviceId;
}

void Protocol::setSessionId(int sessionId) {
	this->sessionId = sessionId;
}

void Protocol::setTotalLen(int totalLen) {
	this->totalLen = totalLen;
}

void Protocol::setUserData(char *userData) {
	this->userData = userData;
}

}
