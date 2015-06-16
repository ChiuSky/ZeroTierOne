/*
 * ZeroTier One - Network Virtualization Everywhere
 * Copyright (C) 2011-2015  ZeroTier, Inc.
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
 *
 * --
 *
 * ZeroTier may be used and distributed under the terms of the GPLv3, which
 * are available at: http://www.gnu.org/licenses/gpl-3.0.html
 *
 * If you would like to embed ZeroTier into a commercial application or
 * redistribute it in a modified binary form, please contact ZeroTier Networks
 * LLC. Start here: http://www.zerotier.com/
 */

#ifndef ZT_TOPOLOGY_HPP
#define ZT_TOPOLOGY_HPP

#include <stdio.h>
#include <string.h>

#include <map>
#include <vector>
#include <stdexcept>
#include <algorithm>

#include "Constants.hpp"

#include "Address.hpp"
#include "Identity.hpp"
#include "Peer.hpp"
#include "Mutex.hpp"
#include "InetAddress.hpp"
#include "Dictionary.hpp"

namespace ZeroTier {

class RuntimeEnvironment;

/**
 * Database of network topology
 */
class Topology
{
public:
	Topology(const RuntimeEnvironment *renv);
	~Topology();

	/**
	 * Set up rootservers for this network
	 * 
	 * @param sn Rootservers for this network
	 */
	void setRootservers(const std::map< Identity,std::vector<InetAddress> > &sn);

	/**
	 * Set up rootservers for this network
	 *
	 * This performs no signature verification of any kind. The caller must
	 * check the signature of the root topology dictionary first.
	 *
	 * @param sn Rootservers dictionary from root-topology
	 */
	void setRootservers(const Dictionary &sn);

	/**
	 * Add a peer to database
	 *
	 * This will not replace existing peers. In that case the existing peer
	 * record is returned.
	 *
	 * @param peer Peer to add
	 * @return New or existing peer (should replace 'peer')
	 */
	SharedPtr<Peer> addPeer(const SharedPtr<Peer> &peer);

	/**
	 * Get a peer from its address
	 * 
	 * @param zta ZeroTier address of peer
	 * @return Peer or NULL if not found
	 */
	SharedPtr<Peer> getPeer(const Address &zta);

	/**
	 * @return Vector of peers that are rootservers
	 */
	inline std::vector< SharedPtr<Peer> > rootserverPeers() const
	{
		Mutex::Lock _l(_lock);
		return _rootserverPeers;
	}

	/**
	 * @return Number of rootservers
	 */
	inline unsigned int numRootservers() const
	{
		Mutex::Lock _l(_lock);
		return (unsigned int)_rootserverPeers.size();
	}

	/**
	 * Get the current favorite rootserver
	 * 
	 * @return Rootserver with lowest latency or NULL if none
	 */
	inline SharedPtr<Peer> getBestRootserver()
	{
		return getBestRootserver((const Address *)0,0,false);
	}

	/**
	 * Get the best rootserver, avoiding rootservers listed in an array
	 * 
	 * This will get the best rootserver (lowest latency, etc.) but will
	 * try to avoid the listed rootservers, only using them if no others
	 * are available.
	 * 
	 * @param avoid Nodes to avoid
	 * @param avoidCount Number of nodes to avoid
	 * @param strictAvoid If false, consider avoided rootservers anyway if no non-avoid rootservers are available
	 * @return Rootserver or NULL if none
	 */
	SharedPtr<Peer> getBestRootserver(const Address *avoid,unsigned int avoidCount,bool strictAvoid);

	/**
	 * @param zta ZeroTier address
	 * @return True if this is a designated rootserver
	 */
	inline bool isRootserver(const Address &zta) const
		throw()
	{
		Mutex::Lock _l(_lock);
		return (std::find(_rootserverAddresses.begin(),_rootserverAddresses.end(),zta) != _rootserverAddresses.end());
	}

	/**
	 * @return Vector of rootserver addresses
	 */
	inline std::vector<Address> rootserverAddresses() const
	{
		Mutex::Lock _l(_lock);
		return _rootserverAddresses;
	}

	/**
	 * Clean and flush database
	 */
	void clean(uint64_t now);

	/**
	 * Apply a function or function object to all peers
	 *
	 * Note: explicitly template this by reference if you want the object
	 * passed by reference instead of copied.
	 *
	 * Warning: be careful not to use features in these that call any other
	 * methods of Topology that may lock _lock, otherwise a recursive lock
	 * and deadlock or lock corruption may occur.
	 *
	 * @param f Function to apply
	 * @tparam F Function or function object type
	 */
	template<typename F>
	inline void eachPeer(F f)
	{
		Mutex::Lock _l(_lock);
		for(std::map< Address,SharedPtr<Peer> >::const_iterator p(_activePeers.begin());p!=_activePeers.end();++p)
			f(*this,p->second);
	}

	/**
	 * @return All currently active peers by address
	 */
	inline std::map< Address,SharedPtr<Peer> > allPeers() const
	{
		Mutex::Lock _l(_lock);
		return _activePeers;
	}

	/**
	 * Validate a root topology dictionary against the identities specified in Defaults
	 *
	 * @param rt Root topology dictionary
	 * @return True if dictionary signature is valid
	 */
	static bool authenticateRootTopology(const Dictionary &rt);

private:
	Identity _getIdentity(const Address &zta);
	void _saveIdentity(const Identity &id);

	const RuntimeEnvironment *RR;

	std::map< Address,SharedPtr<Peer> > _activePeers;
	std::map< Identity,std::vector<InetAddress> > _rootservers;
	std::vector< Address > _rootserverAddresses;
	std::vector< SharedPtr<Peer> > _rootserverPeers;

	Mutex _lock;

	bool _amRootserver;
};

} // namespace ZeroTier

#endif
