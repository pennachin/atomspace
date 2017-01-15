/*
 * opencog/atomspace/AtomSpace.cc
 *
 * Copyright (c) 2008-2010 OpenCog Foundation
 * Copyright (c) 2009, 2013 Linas Vepstas
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <string>
#include <iostream>
#include <fstream>
#include <list>

#include <stdlib.h>

#include <opencog/util/Logger.h>
#include <opencog/util/oc_assert.h>

#include <opencog/atoms/base/ClassServer.h>
#include <opencog/atoms/base/Link.h>
#include <opencog/atoms/base/Node.h>
#include <opencog/atoms/base/types.h>

#include "AtomSpace.h"

//#define DPRINTF printf
#define DPRINTF(...)

using std::string;
using std::cerr;
using std::cout;
using std::endl;
using std::min;
using std::max;
using namespace opencog;

// ====================================================================

/**
 * Transient atomspaces skip some of the initialization steps,
 * so that they can be constructed more quickly.  Transient atomspaces
 * are typically used as scratch spaces, to hold temporary results
 * during evaluation, pattern matching and inference. Such temporary
 * spaces don't need some of the heavier-weight crud that atomspaces
 * are festooned with.
 */
AtomSpace::AtomSpace(AtomSpace* parent, bool transient) :
    _atom_table(parent? &parent->_atom_table : NULL, this, transient),
    _backing_store(NULL),
    _transient(transient)
{
}

AtomSpace::~AtomSpace()
{
}

AtomSpace::AtomSpace(const AtomSpace&) :
    _atom_table(NULL),
    _backing_store(NULL)
{
     throw opencog::RuntimeException(TRACE_INFO,
         "AtomSpace - Cannot copy an object of this class");
}

void AtomSpace::ready_transient(AtomSpace* parent)
{
    _atom_table.ready_transient(parent? &parent->_atom_table : NULL, this);
}

void AtomSpace::clear_transient()
{
    _atom_table.clear_transient();
}

AtomSpace& AtomSpace::operator=(const AtomSpace&)
{
     throw opencog::RuntimeException(TRACE_INFO,
         "AtomSpace - Cannot copy an object of this class");
}

bool AtomSpace::compare_atomspaces(const AtomSpace& space_first,
                                   const AtomSpace& space_second,
                                   bool check_truth_values,
                                   bool emit_diagnostics)
{
    // Compare sizes
    if (space_first.get_size() != space_second.get_size())
    {
        if (emit_diagnostics)
            std::cout << "compare_atomspaces - size " << 
                    space_first.get_size() << " != size " << 
                    space_second.get_size() << std::endl;
        return false;
    }

    // Compare node count
    if (space_first.get_num_nodes() != space_second.get_num_nodes())
    {
        if (emit_diagnostics)
            std::cout << "compare_atomspaces - node count " << 
                    space_first.get_num_nodes() << " != node count " << 
                    space_second.get_num_nodes() << std::endl;
        return false;
    }

    // Compare link count
    if (space_first.get_num_links() != space_second.get_num_links())
    {
        if (emit_diagnostics)
            std::cout << "compare_atomspaces - link count " << 
                    space_first.get_num_links() << " != link count " << 
                    space_second.get_num_links() << std::endl;
        return false;
    }

    // If we get this far, we need to compare each individual atom.

    // Get the atoms in each atomspace.
    HandleSeq atomsInFirstSpace, atomsInSecondSpace;
    space_first.get_all_atoms(atomsInFirstSpace);
    space_second.get_all_atoms(atomsInSecondSpace);

    // Uncheck each atom in the second atomspace.
    for (auto atom : atomsInSecondSpace)
        atom->setUnchecked();

    // Loop to see if each atom in the first has a match in the second.
    const AtomTable& table_second = space_second._atom_table;
    for (auto atom_first : atomsInFirstSpace)
    {
        Handle atom_second = table_second.getHandle(atom_first);

        if( false)
        {
        Handle atom_second;
        if (atom_first->isNode())
        {
            atom_second = table_second.getHandle(atom_first->getType(),
                        atom_first->getName());
        }
        else if (atom_first->isLink())
        {
            atom_second =  table_second.getHandle(atom_first->getType(),
                        atom_first->getOutgoingSet());
        }
        else
        {
             throw opencog::RuntimeException(TRACE_INFO,
                 "AtomSpace::compare_atomspaces - atom not Node or Link");
        }
        }

        // If the atoms don't match because one of them is NULL.
        if ((atom_first and not atom_second) or
            (atom_second and not atom_first))
        {
            if (emit_diagnostics)
            {
                if (atom_first)
                    std::cout << "compare_atomspaces - first atom " << 
                            atom_first->toString() << " != NULL " << 
                            std::endl;
                if (atom_second)
                    std::cout << "compare_atomspaces - first atom "  << 
                            "NULL != second atom " << 
                            atom_second->toString() << std::endl;
            }
            return false;
        }

        // If the atoms don't match... Compare the atoms not the pointers
        // which is the default if we just use Handle operator ==.
        if (*((AtomPtr) atom_first) != *((AtomPtr) atom_second))
        {
            if (emit_diagnostics)
                std::cout << "compare_atomspaces - first atom " << 
                        atom_first->toString() << " != second atom " << 
                        atom_second->toString() << std::endl;
            return false;
        }

        // Check the truth values...
        if (check_truth_values)
        {
            TruthValuePtr truth_first = atom_first->getTruthValue();
            TruthValuePtr truth_second = atom_second->getTruthValue();
            if (*truth_first != *truth_second)
            {
                if (emit_diagnostics)
                    std::cout << "compare_atomspaces - first truth " << 
                            atom_first->toString() << " != second truth " << 
                            atom_second->toString() << std::endl;
                return false;
            }
        }

        // Set the check for the second atom.
        atom_second->setChecked();
    }

    // Make sure each atom in the second atomspace has been checked.
    bool all_checked = true;
    for (auto atom : atomsInSecondSpace)
    {
        if (!atom->isChecked())
        {
            if (emit_diagnostics)
                std::cout << "compare_atomspaces - unchecked space atom " << 
                        atom->toString() << std::endl;
            all_checked = false;
        }
    }
    if (!all_checked)
        return false;

    // If we get this far, then the spaces are equal.
    return true;
}

bool AtomSpace::operator==(const AtomSpace& other) const
{
    return compare_atomspaces(*this, other, CHECK_TRUTH_VALUES, 
            DONT_EMIT_DIAGNOSTICS);
}

bool AtomSpace::operator!=(const AtomSpace& other) const
{
    return not operator==(other);
}


// ====================================================================

void AtomSpace::registerBackingStore(BackingStore *bs)
{
    _backing_store = bs;
}

void AtomSpace::unregisterBackingStore(BackingStore *bs)
{
    if (bs == _backing_store) _backing_store = NULL;
}

// ====================================================================

Handle AtomSpace::add_atom(const Handle& h, bool async)
{
    // If it is a DeleteLink, then the addition will fail. Deal with it.
    Handle rh;
    try {
        rh = _atom_table.add(h, async);
    }
    catch (const DeleteException& ex) {
        // Atom deletion has not been implemented in the backing store
        // This is a major to-do item.
        if (_backing_store)
// Under construction ....
	        throw RuntimeException(TRACE_INFO, "Not implemented!!!");
    }
    return rh;
}

Handle AtomSpace::add_node(Type t, const string& name,
                           bool async)
{
    return _atom_table.add(createNode(t, name), async);
}

Handle AtomSpace::get_node(Type t, const string& name)
{
    return _atom_table.getHandle(t, name);
}

Handle AtomSpace::add_link(Type t, const HandleSeq& outgoing, bool async)
{
    // If it is a DeleteLink, then the addition will fail. Deal with it.
    Handle rh;
    try {
        rh = _atom_table.add(createLink(t, outgoing), async);
    }
    catch (const DeleteException& ex) {
        // Atom deletion has not been implemented in the backing store
        // This is a major to-do item.
        if (_backing_store)
// Under construction ....
	        throw RuntimeException(TRACE_INFO, "Not implemented!!!");
    }
    return rh;
}

Handle AtomSpace::get_link(Type t, const HandleSeq& outgoing)
{
    return _atom_table.getHandle(t, outgoing);
}

void AtomSpace::store_atom(const Handle& h)
{
    if (NULL == _backing_store)
        throw RuntimeException(TRACE_INFO, "No backing store");

    _backing_store->storeAtom(h);
}

Handle AtomSpace::fetch_atom(Handle& h)
{
    if (nullptr == _backing_store)
        throw RuntimeException(TRACE_INFO, "No backing store");
    if (nullptr == h) return Handle::UNDEFINED;

    // We deal with two distinct cases.
    // 1) If atom table already knows about this atom, then this
    //    function returns the atom-table's version of the atom.
    //    In particular, no attempt is made to reconcile the possibly
    //    differing truth values in the atomtable vs. backing store.
    //    Why?  Because it is likely that the user plans to over-write
    //    what is in the backend.
    // 2) If (1) does not hold, i.e. the atom is not in this table, nor
    //    it's environs, then assume that atom is from some previous
    //    (recursive) query; do fetch it from backing store (i.e. fetch
    //    the TV) and add it to the atomtable.
    // For case 2, if the atom is a link, then it's outgoing set is
    // fetched as well, as currently, a link cannot be added to the
    // atomtable, unless all of its outgoing set already is in the
    // atomtable.

    // Case 1:
    Handle hb(_atom_table.getHandle(h));
    if (_atom_table.holds(hb))
        return hb;

    // Case 2:
    // This atom is not yet in any (this??) atomspace; go get it.
    if (NULL == h->getAtomTable()) {
        TruthValuePtr tv;
        if (h->isNode()) {
            tv = _backing_store->getNode(h->getType(),
                                         h->getName().c_str());
        }
        else if (h->isLink()) {
            tv = _backing_store->getLink(h);
        }

        // If we still don't have an atom, then the requested atom
        // was "insane", that is, unknown by either the atom table
        // (case 1) or the backend.
        if (NULL == tv)
            throw RuntimeException(TRACE_INFO,
                "Asked backend for an atom %s\n",
                h->toString().c_str());
        h->setTruthValue(tv);
    }

    return _atom_table.add(h, false);
}

Handle AtomSpace::fetch_incoming_set(Handle h, bool recursive)
{
    if (nullptr == _backing_store)
        throw RuntimeException(TRACE_INFO, "No backing store");

    h = get_atom(h);

    if (nullptr == h) return Handle::UNDEFINED;

    // Get everything from the backing store.
    HandleSeq iset = _backing_store->getIncomingSet(h);
    size_t isz = iset.size();
    for (size_t i=0; i<isz; i++) {
        Handle hi(iset[i]);
        if (recursive) {
            fetch_incoming_set(hi, true);
        } else {
            add_atom(hi);
        }
    }
    return h;
}

bool AtomSpace::remove_atom(Handle h, bool recursive)
{
    if (_backing_store) {
        // Atom deletion has not been implemented in the backing store
        // This is a major to-do item.
// Under construction ....
        throw RuntimeException(TRACE_INFO, "Not implemented!!!");
    }
    return 0 < _atom_table.extract(h, recursive).size();
}

std::string AtomSpace::to_string() const
{
	std::stringstream ss;
	ss << *this;
	return ss.str();
}

namespace std {

ostream& operator<<(ostream& out, const opencog::AtomSpace& as) {
    list<opencog::Handle> results;
    as.get_handles_by_type(back_inserter(results), opencog::ATOM, true);
    for (const opencog::Handle& h : results)
	    if (h->getIncomingSetSize() == 0)
		    out << h->toString() << endl;
    return out;
}

} // namespace std
