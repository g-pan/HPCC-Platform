/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems®.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
############################################################################## */

// Keybuild script for generated by AIKEY

record_sample := RECORD
  string2              src;
  string6              dt_first_seen;
  string6              dt_last_seen;
  string6              tu_date;
  string1              rec_type;
  string10             phone;
  string6              dob;
  string5              title;
  string20             fname;
  string1              minit;
  string20             lname;
  string5              name_suffix;
  string20             fname_2;
  string1              minit_2;
  string20             lname_2;
  string5              name_suffix_2;
  string20             fname_3;
  string1              minit_3;
  string20             lname_3;
  string5              name_suffix_3;
  string20             fname_4;
  string1              minit_4;
  string20             lname_4;
  string5              name_suffix_4;
  string10             prim_range;
  string2              predir;
  string28             prim_name;
  string4              suffix;
  string2              postdir;
  string10             unit_desig;
  string8              sec_range;
  string25             city_name;
  string2              st;
  string5              zip;
  string4              zip4;
  string3              county;
  string4              city;
END;
rawfile := DATASET('sample', record_sample, THOR);

// Drop fields from input that are not used in any keys in this section

record_sample_st_city_fname_lname_minit := RECORD
  string2              st;
  string4              city;
  string20             fname;
  string20             lname;
  string1              minit;
  big_endian unsigned integer8 __filepos;
END;

record_sample_st_city_fname_lname_minit drops1(rawfile L) := TRANSFORM SELF := L;  END;
ProjectCover1 := PROJECT(rawfile, drops1(left));

sort7 := SORT(ProjectCover1, st, city);

// Partial sort to do next key

sort4 := SORT(GROUP(sort7, st, city, LOCAL), fname, lname, __filepos, LOCAL);

// build a key

OUTPUT(sort4, { st; city; fname; lname;  __filepos} ,'keyfile');

// Partial sort to do next key

sort5 := SORT(GROUP(sort7, st, city, LOCAL), lname, fname, minit, __filepos, LOCAL);

// build a key

OUTPUT(sort5, { st; city; lname; fname; minit;  __filepos} ,'keyfile');

sort8 := SORT(ProjectCover1, st, lname, fname, minit);

// build a key

OUTPUT(sort8, { st; lname; fname; minit;  __filepos} ,'keyfile');

