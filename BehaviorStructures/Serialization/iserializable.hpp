#pragma once
#include "json/single_include/nlohmann/json.hpp"
class ISerializable
{

#if defined(NLOHMANN_JSON_VERSION_MAJOR) 
public:
	virtual nlohmann::json Serialize() = 0;
	virtual void Deserialize(nlohmann::json& json) = 0;
#endif

};

