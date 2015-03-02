//
//  IDDiskArbitrationUtils.cpp
//  InvariantDisks
//
//  Created by Gerhard Röthlin on 2014.04.27.
//  Copyright (c) 2014 the-color-black.net. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without modification, are permitted
//  provided that the conditions of the "3-Clause BSD" license described in the BSD.LICENSE file are met.
//  Additional licensing options are described in the README file.
//

#include "IDDiskArbitrationUtils.hpp"

namespace ID
{
	std::ostream & operator<<(std::ostream & os, DADiskRef disk)
	{
		return os << getDiskInformation(disk);
	}

	std::ostream & operator<<(std::ostream & os, DiskInformation const & disk)
	{
		return os << "Disk: (\n"
			<< "\tVolumeKind=\"" << disk.volumeKind << "\"\n"
			<< "\tVolumeUUID=\"" << disk.volumeUUID << "\"\n"
			<< "\tVolumeName=\"" << disk.volumeName << "\"\n"
			<< "\tVolumePath=\"" << disk.volumePath << "\"\n"
			<< "\tMediaKind=\"" << disk.mediaKind << "\"\n"
			<< "\tMediaType=\"" << disk.mediaType  << "\"\n"
			<< "\tMediaUUID=\"" << disk.mediaUUID << "\"\n"
			<< "\tMediaBSDName=\"" << disk.mediaBSDName << "\"\n"
			<< "\tMediaName=\"" << disk.mediaName << "\"\n"
			<< "\tMediaPath=\"" << disk.mediaPath << "\"\n"
			<< "\tMediaContent=\"" << disk.mediaContent << "\"\n"
			<< "\tMedia(Whole,Leaf,Writable)=(" << disk.mediaWhole << ", "
				<< disk.mediaLeaf << ", " << disk.mediaWritable << ")\n"
			<< "\tDeviceGUID=\"" << disk.deviceGUID << "\"\n"
			<< "\tDevicePath=\"" << disk.devicePath << "\"\n"
			<< "\tDeviceModel=\"" << disk.deviceModel << "\"\n"
			<< "\tBusName=\"" << disk.busName << "\"\n"
			<< "\tBusPath=\"" << disk.busPath << "\"\n"
			<< "\tIOSerial=\"" << disk.ioSerial << "\"\n"
			<< ")";
	}

	std::string to_string(CFStringRef str)
	{
		std::string result;
		CFRange strRange = CFRangeMake(0, CFStringGetLength(str));
		CFIndex strBytes = 0;
		CFStringGetBytes(str, strRange, kCFStringEncodingUTF8, 0, false, nullptr, 0, &strBytes);
		if (strBytes > 0)
		{
			result.resize(static_cast<size_t>(strBytes), '\0');
			CFStringGetBytes(str, strRange, kCFStringEncodingUTF8, 0, false,
							 reinterpret_cast<UInt8*>(&result[0]), strBytes, nullptr);
		}
		return result;
	}

	std::string to_string(CFURLRef url)
	{
		CFStringRef str = CFURLCopyPath(url);
		std::string result = to_string(str);
		CFRelease(str);
		return result;
	}

	std::string to_string(CFDataRef data)
	{
		return std::string(reinterpret_cast<char const *>(CFDataGetBytePtr(data)), CFDataGetLength(data));
	}

	std::string to_string(CFUUIDRef uuid)
	{
		CFStringRef str = CFUUIDCreateString(kCFAllocatorDefault, uuid);
		std::string result = to_string(str);
		CFRelease(str);
		return result;
	}

	std::string to_string(CFTypeRef variant)
	{
		if (!variant)
			return std::string();
		CFTypeID typeID = CFGetTypeID(variant);
		if (typeID == CFStringGetTypeID())
			return to_string(CFStringRef(variant));
		else if (typeID == CFURLGetTypeID())
			return to_string(CFURLRef(variant));
		else if (typeID == CFDataGetTypeID())
			return to_string(CFDataRef(variant));
		else if (typeID == CFUUIDGetTypeID())
			return to_string(CFUUIDRef(variant));
		return std::string();
	}

	template<typename T>
	std::string stringFromDictionary(CFDictionaryRef dict, CFStringRef key)
	{
		if (T value = static_cast<T>(CFDictionaryGetValue(dict, key)))
			return to_string(value);
		return std::string();
	}

	int64_t numberFromDictionary(CFDictionaryRef dict, CFStringRef key)
	{
		if (CFNumberRef value = static_cast<CFNumberRef>(CFDictionaryGetValue(dict, key)))
		{
			int64_t number = 0;
			CFNumberGetValue(value, kCFNumberSInt64Type, &number);
			return number;
		}
		return 0;
	}

	bool boolFromDictionary(CFDictionaryRef dict, CFStringRef key)
	{
		if (CFBooleanRef value = static_cast<CFBooleanRef>(CFDictionaryGetValue(dict, key)))
		{
			return CFBooleanGetValue(value);
		}
		return false;
	}

	std::string stringFromIOObjectWithParents(io_object_t ioObject, CFStringRef key)
	{
		std::string result;
		CFTypeRef serial = IORegistryEntrySearchCFProperty(ioObject, kIOServicePlane, key,
			kCFAllocatorDefault, kIORegistryIterateRecursively | kIORegistryIterateParents);
		if (serial)
		{
			result = to_string(serial);
			CFRelease(serial);
		}
		return result;
	}

	std::string serialNumberFromIOObject(io_object_t ioObject)
	{
		static CFStringRef const serialStrings[] = {
			CFSTR("Serial Number"),
			CFSTR("INQUIRY Unit Serial Number"),
			CFSTR("USB Serial Number")
		};
		for (CFStringRef serialString: serialStrings)
		{
			std::string serial = stringFromIOObjectWithParents(ioObject, serialString);
			if (!serial.empty())
				return serial;
		}
		return std::string();
	}

	DiskInformation getDiskInformation(DADiskRef disk)
	{
		DiskInformation info;
		// DiskArbitration
		CFDictionaryRef descDict = DADiskCopyDescription(disk);
		info.volumeKind = stringFromDictionary<CFStringRef>(descDict, kDADiskDescriptionVolumeKindKey);
		info.volumeUUID = stringFromDictionary<CFUUIDRef>(descDict, kDADiskDescriptionVolumeUUIDKey);
		info.volumeName = stringFromDictionary<CFStringRef>(descDict, kDADiskDescriptionVolumeNameKey);
		info.volumePath = stringFromDictionary<CFURLRef>(descDict, kDADiskDescriptionVolumePathKey);
		info.mediaKind = stringFromDictionary<CFStringRef>(descDict, kDADiskDescriptionMediaKindKey);
		info.mediaType = stringFromDictionary<CFStringRef>(descDict, kDADiskDescriptionMediaTypeKey);
		info.mediaUUID = stringFromDictionary<CFUUIDRef>(descDict, kDADiskDescriptionMediaUUIDKey);
		info.mediaBSDName = stringFromDictionary<CFStringRef>(descDict, kDADiskDescriptionMediaBSDNameKey);
		info.mediaName = stringFromDictionary<CFStringRef>(descDict, kDADiskDescriptionMediaNameKey);
		info.mediaPath = stringFromDictionary<CFStringRef>(descDict, kDADiskDescriptionMediaPathKey);
		info.mediaContent = stringFromDictionary<CFStringRef>(descDict, kDADiskDescriptionMediaContentKey);
		info.mediaWhole = boolFromDictionary(descDict, kDADiskDescriptionMediaWholeKey);
		info.mediaLeaf = boolFromDictionary(descDict, kDADiskDescriptionMediaLeafKey);
		info.mediaWritable = boolFromDictionary(descDict, kDADiskDescriptionMediaWritableKey);
		info.deviceGUID = stringFromDictionary<CFDataRef>(descDict, kDADiskDescriptionDeviceGUIDKey);
		info.devicePath = stringFromDictionary<CFStringRef>(descDict, kDADiskDescriptionDevicePathKey);
		info.deviceModel = stringFromDictionary<CFStringRef>(descDict, kDADiskDescriptionDeviceModelKey);
		info.busName = stringFromDictionary<CFStringRef>(descDict, kDADiskDescriptionBusNameKey);
		info.busPath = stringFromDictionary<CFStringRef>(descDict, kDADiskDescriptionBusPathKey);
		CFRelease(descDict);
		// IOKit
		io_service_t io = DADiskCopyIOMedia(disk);
		info.ioSerial = serialNumberFromIOObject(io);
		CFMutableDictionaryRef ioDict = nullptr;
		if (IORegistryEntryCreateCFProperties(io, &ioDict, kCFAllocatorDefault, 0) == kIOReturnSuccess)
		{
			// TODO: Pick out useful IOKit properties
			CFRelease(ioDict);
		}
		IOObjectRelease(io);
		return info;
	}
}