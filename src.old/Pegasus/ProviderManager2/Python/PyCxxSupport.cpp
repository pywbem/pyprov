/*****************************************************************************
* (C) Copyright 2007 Novell, Inc. 
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 2 of the
* License, or (at your option) any later version.
*   
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*   
* You should have received a copy of the GNU Lesser General Public
* License along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*****************************************************************************/
//-----------------------------------------------------------------------------
//
// Copyright (c) 1998 - 2007, The Regents of the University of California
// Produced at the Lawrence Livermore National Laboratory
// All rights reserved.
//
// This file is part of PyCXX. For details,see http://cxx.sourceforge.net/. The
// full copyright notice is contained in the file COPYRIGHT located at the root
// of the PyCXX distribution.
//
// Redistribution  and  use  in  source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
//  - Redistributions of  source code must  retain the above  copyright notice,
//    this list of conditions and the disclaimer below.
//  - Redistributions in binary form must reproduce the above copyright notice,
//    this  list of  conditions  and  the  disclaimer (as noted below)  in  the
//    documentation and/or materials provided with the distribution.
//  - Neither the name of the UC/LLNL nor  the names of its contributors may be
//    used to  endorse or  promote products derived from  this software without
//    specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR  IMPLIED WARRANTIES, INCLUDING,  BUT NOT  LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR  PURPOSE
// ARE  DISCLAIMED.  IN  NO  EVENT  SHALL  THE  REGENTS  OF  THE  UNIVERSITY OF
// CALIFORNIA, THE U.S.  DEPARTMENT  OF  ENERGY OR CONTRIBUTORS BE  LIABLE  FOR
// ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT  LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR
// SERVICES; LOSS OF  USE, DATA, OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER
// CAUSED  AND  ON  ANY  THEORY  OF  LIABILITY,  WHETHER  IN  CONTRACT,  STRICT
// LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY  WAY
// OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
//-----------------------------------------------------------------------------

#include "PyCxxObjects.h"
namespace Py
{

Py_UNICODE unicode_null_string[1] = { 0 };


// TMM: non-member operaters for iterators - see above
// I've also made a bug fix in respect to the cxx code
// (dereffed the left.seq and right.seq comparison)
bool operator==(const Sequence::iterator& left, const Sequence::iterator& right)
{
    return left.eql( right );
}

bool operator!=(const Sequence::iterator& left, const Sequence::iterator& right)
{
    return left.neq( right );
}

bool operator< (const Sequence::iterator& left, const Sequence::iterator& right)
{
    return left.lss( right );
}

bool operator> (const Sequence::iterator& left, const Sequence::iterator& right)
{
    return left.gtr( right );
}

bool operator<=(const Sequence::iterator& left, const Sequence::iterator& right)
{
    return left.leq( right );
}

bool operator>=(const Sequence::iterator& left, const Sequence::iterator& right)
{
    return left.geq( right );
}

// now for const_iterator
bool operator==(const Sequence::const_iterator& left, const Sequence::const_iterator& right)
{
    return left.eql( right );
}

bool operator!=(const Sequence::const_iterator& left, const Sequence::const_iterator& right)
{
    return left.neq( right );
}

bool operator< (const Sequence::const_iterator& left, const Sequence::const_iterator& right)
{
    return left.lss( right );
}

bool operator> (const Sequence::const_iterator& left, const Sequence::const_iterator& right)
{
    return left.gtr( right );
}

bool operator<=(const Sequence::const_iterator& left, const Sequence::const_iterator& right)
{
    return left.leq( right );
}

bool operator>=(const Sequence::const_iterator& left, const Sequence::const_iterator& right)
{
    return left.geq( right );
}

// For mappings:
bool operator==(const Mapping::iterator& left, const Mapping::iterator& right)
{
    return left.eql( right );
}

bool operator!=(const Mapping::iterator& left, const Mapping::iterator& right)
{
    return left.neq( right );
}

// now for const_iterator
bool operator==(const Mapping::const_iterator& left, const Mapping::const_iterator& right)
{
    return left.eql( right );
}

bool operator!=(const Mapping::const_iterator& left, const Mapping::const_iterator& right)
{
    return left.neq( right );
}

} // Py
