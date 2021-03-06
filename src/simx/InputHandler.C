// Copyright (c) 2012. Los Alamos National Security, LLC. 

// This material was produced under U.S. Government contract DE-AC52-06NA25396
// for Los Alamos National Laboratory (LANL), which is operated by Los Alamos 
// National Security, LLC for the U.S. Department of Energy. The U.S. Government 
// has rights to use, reproduce, and distribute this software.  

// NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY, 
// EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  
// If software is modified to produce derivative works, such modified software should
// be clearly marked, so as not to confuse it with the version available from LANL.

// Additionally, this library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License v 2.1 as published by the 
// Free Software Foundation. Accordingly, this library is distributed in the hope that 
// it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of 
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See LICENSE.txt for more details.

//--------------------------------------------------------------------------
// File:    InputHandler.C
// Module:  simx
// Author:  Lukas Kroc, Sunil Thulasidasan
// Created: Feb 7 2005
//
// Description: Create and manage inputs
//
// @@
//
//--------------------------------------------------------------------------

#include "simx/InputHandler.h"
#include "simx/Entity.h"
#include "simx/logger.h"

#include "Common/Assert.h"
#include "Config/Configuration.h"

#include "boost/lexical_cast.hpp"
//#include <boost/make_shared.hpp>
#include <list>
#include <map>

using namespace std;
using namespace Config;

namespace simx {

/// explicit instantiation of templates to use
template class InputHandler<int>;
template class InputHandler<std::string>;


template<typename ObjectIdent>
void InputHandler<ObjectIdent>::loadProfile( const ProfileID profileId, boost::shared_ptr<Input> input )
{
  
    Logger::debug3() << "InputHandler: loading profile " << profileId << endl;
    
    // obtain the appropriate profile set (contains all profileIds in that set)
    const Config::Configuration::ConfigSet& cset = gConfig.GetConfigurationSet(fProfileSetName);
    if (cset.size() == 0)
    {	
	Logger::warn() << "InputHandler: no config set " << fProfileSetName << ", no profiles made" 
	    << " for profileId " << profileId << endl;
	return;
    }

    // turn the set into a map ProfileID->profile subset
    // TODO: this really is needed only once per run
    map<std::string, Config::Configuration::ConfigMap*>	profileMap;
    for (Config::Configuration::ConfigSet::iterator it = cset.begin();
	it != cset.end();
	++it)
    {
	if( !profileMap.insert(*it).second )
	{
	    Logger::warn() << "InputHandler: profile " << it->first << " occurs multiple times in set " 
		<< fProfileSetName << endl;
	}
    }

    // make sure you follow through PARENT profile hiesrarchy (a tree structure)
    // make a list of Profiles so that the very top-one in the hierarchy is first etc...
    list<Config::Configuration::ConfigMap*>	profileList;
    string profileStr = boost::lexical_cast<string>(profileId);
    while( true )
    {
	Config::Configuration::ConfigMap* profilePtr = profileMap[ profileStr ];
	
	if( !profilePtr )
	{
	    Logger::error() << "InputHandler: profile " << profileStr << " does not exist in set " 
		<< fProfileSetName << endl;
	    break;
	}
	// add the profile to the list
	Logger::debug3() << "InputHandler: adding " << profileStr << " to the profileList" << endl;
	profileList.push_front( profilePtr );

	// look for its parent
	Config::Configuration::ConfigMap::const_iterator par_it = profilePtr->find("PARENT");
	if( par_it == profilePtr->end() )
	{
	    // end of profile hierarchy
	    break;
	}
	
	profileStr = par_it->second;
    }
    
    ProfileHolder profileHolder;	// this object will have all name-value pairs from the 
					// whole profile hierarchy, descendants overwriting 
					// parent values
    // now go through the list and initialize the profileSource input by
    // merging all subsets, following the PARENT hierarchy from top to bottom
    for(list<Config::Configuration::ConfigMap*>::iterator iter = profileList.begin();
	iter != profileList.end();
	++iter)
    {
	SMART_ASSERT( *iter );
	profileHolder.add( (*iter)->begin(), (*iter)->end() );
    }
    
    // finally, read in the profile
    SMART_ASSERT( input );
    input->readProfile( profileHolder );
}

 

} // namespace

