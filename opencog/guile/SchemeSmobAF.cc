/*
 * opencog/guile/SchemeSmobAF.cc
 *
 * Scheme small objects (SMOBS) for AttentionalFocus and AttentionalFocusBoundary.
 *
 * Copyright (C) 2014 Cosmo Harrigan
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

#include <cstddef>
#include <libguile.h>

#include <opencog/guile/SchemeSmob.h>
#include <opencog/attentionbank/AttentionBank.h>

using namespace opencog;

/**
 * Return AttentionalFocus Boundary
 */
SCM SchemeSmob::ss_af_boundary (void)
{
	return scm_from_short(attentionbank().getAttentionalFocusBoundary());
}

/**
 * Set AttentionalFocus Boundary
 */
SCM SchemeSmob::ss_set_af_boundary (SCM sboundary)
{
	if (scm_is_false(scm_integer_p(sboundary)))
		scm_wrong_type_arg_msg("cog-set-af-boundary", 1, sboundary,
			"integer opencog AttentionalFocus Boundary");

	short bdy = scm_to_short(sboundary);
	return scm_from_short(attentionbank().setAttentionalFocusBoundary(bdy));
}

/**
 * Return the list of atoms in the AttentionalFocus
 */
SCM SchemeSmob::ss_af (void)
{
    HandleSeq attentionalFocus;
    attentionbank().get_handle_set_in_attentional_focus(back_inserter(attentionalFocus));
    size_t isz = attentionalFocus.size();
	if (0 == isz) return SCM_EOL;

    SCM head = SCM_EOL;
    for (size_t i = 0; i < isz; i++) {
        Handle hi = attentionalFocus[i];
        SCM smob = handle_to_scm(hi);
        head = scm_cons(smob, head);
    }

	return head;
}
