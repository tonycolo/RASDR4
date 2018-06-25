//
// Created by astro on 3/23/18.
//

#include "Properties.h"

Properties::Properties(const std::string & path) {
   properties = new Poco::Util::PropertyFileConfiguration(path);
}

Poco::Util::PropertyFileConfiguration* Properties::getPropertyFileConfiguration() {
   return properties;
}

