/*
 * opencog/atoms/reduct/PlusLink.cc
 *
 * Copyright (C) 2015 Linas Vepstas
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

#include <opencog/atoms/base/atom_types.h>
#include <opencog/atoms/base/ClassServer.h>
#include <opencog/atoms/NumberNode.h>
#include "PlusLink.h"
#include "TimesLink.h"

using namespace opencog;

PlusLink::PlusLink(const HandleSeq& oset, TruthValuePtr tv)
    : ArithmeticLink(PLUS_LINK, oset, tv)
{
	init();
}

PlusLink::PlusLink(Type t, const HandleSeq& oset, TruthValuePtr tv)
    : ArithmeticLink(t, oset, tv)
{
	if (not classserver().isA(t, PLUS_LINK))
		throw InvalidParamException(TRACE_INFO, "Expecting a PlusLink");
	init();
}

PlusLink::PlusLink(const Handle& a, const Handle& b,
                   TruthValuePtr tv)
    : ArithmeticLink(PLUS_LINK, a, b, tv)
{
	init();
}

PlusLink::PlusLink(Type t, const Handle& a, const Handle& b,
                   TruthValuePtr tv)
    : ArithmeticLink(t, a, b, tv)
{
	if (not classserver().isA(t, PLUS_LINK))
		throw InvalidParamException(TRACE_INFO, "Expecting a PlusLink");
	init();
}

PlusLink::PlusLink(Link& l)
    : ArithmeticLink(l)
{
	Type tscope = l.getType();
	if (not classserver().isA(tscope, PLUS_LINK))
		throw InvalidParamException(TRACE_INFO, "Expecting a PlusLink");
	init();
}

void PlusLink::init(void)
{
	knild = 0.0;
	knil = Handle(createNumberNode("0"));

	distributive_type = TIMES_LINK;
}

// ============================================================

double PlusLink::konsd(double a, double b) const { return a+b; }

static inline double get_double(const Handle& h)
{
	NumberNodePtr nnn(NumberNodeCast(h));
	if (NULL == nnn)
		nnn = createNumberNode(*NodeCast(h));

	return nnn->get_value();
}

// ============================================================

Handle PlusLink::kons(const Handle& fi, const Handle& fj)
{
	// Are they numbers?
	if (NUMBER_NODE == fi->getType() and
	    NUMBER_NODE == fj->getType())
	{
		double sum = get_double(fi) + get_double(fj);
		return Handle(createNumberNode(sum));
	}

	// Is fi identical to fj? If so, then replace by 2*fi
	if (fi == fj)
	{
		Handle two(createNumberNode("2"));
		return Handle(createTimesLink(fi, two));
	}

	// If j is (TimesLink x a) and i is identical to x,
	// then create (TimesLink x (a+1))
	//
	// If j is (TimesLink x a) and i is (TimesLink x b)
	// then create (TimesLink x (a+b))
	//
	if (fj->getType() == TIMES_LINK)
	{
		bool do_add = false;
		HandleSeq rest;

		Handle exx = fj->getOutgoingAtom(0);

		// Handle the (a+1) case described above.
		if (fi == exx)
		{
			Handle one(createNumberNode("1"));
			rest.push_back(one);
			do_add = true;
		}

		// Handle the (a+b) case described above.
		else if (fi->getType() == TIMES_LINK and
		         fi->getOutgoingAtom(0) == exx)
		{
			const HandleSeq& ilpo = fi->getOutgoingSet();
			size_t ilpsz = ilpo.size();
			for (size_t k=1; k<ilpsz; k++)
				rest.push_back(ilpo[k]);
			do_add = true;
		}

		if (do_add)
		{
			const HandleSeq& jlpo = fj->getOutgoingSet();
			size_t jlpsz = jlpo.size();
			for (size_t k=1; k<jlpsz; k++)
				rest.push_back(jlpo[k]);

			// a_plus is now (a+1) or (a+b) as described above.
			// We need to insert into the atomspace, else reduce() horks
			// up the knil compares during reduction.
			Handle foo(createLink(PLUS_LINK, rest));
			if (_atomTable)
				foo = _atomTable->getAtomSpace()->add_atom(foo);

			PlusLinkPtr ap = PlusLinkCast(foo);
			Handle a_plus(ap->reduce());

			return Handle(createTimesLink(exx, a_plus));
		}
	}

	// If we are here, we've been asked to add two things of the same
	// type, but they are not of a type that we know how to add.
	// For example, fi and fj might be two different VariableNodes.
	return Handle(createPlusLink(fi, fj)->reorder());
}

// ============================================================
