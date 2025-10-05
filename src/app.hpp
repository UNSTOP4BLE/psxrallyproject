#pragma once 

#include "engine/common.hpp"

class APP {
public:
  	ENGINE::COMMON::UniquePtr<ENGINE::COMMON::Scene> curscene;
};

extern APP g_app;