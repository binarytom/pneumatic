/**
 * @file pmat.cpp
 * @author Tom Molesworth <tom@audioboundary.com>
 * @date 08/04/15 22:10:50
 *
 * 
 * $Id$
 */

#include <string>
#include <unordered_map>

class Lookup {
public:
	Lookup();

	std::string root_desc(const std::string &name) const { return root_.at(name); }

private:
	std::unordered_map<std::string, std::string> root_;
};

Lookup::Lookup(
):root_{ }
{
	root_["main_cv"] = u8"the main code";
	root_["defstash"] = u8"the default stash";
	root_["mainstack"] = u8"the main stack AV";
	root_["beginav"] = u8"the BEGIN list";
	root_["checkav"] = u8"the CHECK list";
	root_["unitcheckav"] = u8"the UNITCHECK list";
	root_["initav"] = u8"the INIT list";
	root_["endav"] = u8"the END list";
	root_["strtab"] = u8"the shared string table HV";
	root_["envgv"] = u8"the ENV GV";
	root_["incgv"] = u8"the INC GV";
	root_["statgv"] = u8"the stat GV";
	root_["statname"] = u8"the statname SV";
	root_["tmpsv"] = u8"the temporary SV";
	root_["defgv"] = u8"the default GV";
	root_["argvgv"] = u8"the ARGV GV";
	root_["argoutgv"] = u8"the argvout GV";
	root_["argvout_stack"] = u8"the argout stack AV";
	root_["fdpidav"] = u8"the FD-to-PID mapping AV";
	root_["preambleav"] = u8"the compiler preamble AV";
	root_["modglobalhv"] = u8"the module data globals HV";
	root_["regex_padav"] = u8"the REGEXP pad AV";
	root_["sortstash"] = u8"the sort stash";
	root_["firstgv"] = u8"the *a GV";
	root_["secondgv"] = u8"the *b GV";
	root_["debstash"] = u8"the debugger stash";
	root_["stashcache"] = u8"the stash cache";
	root_["isarev"] = u8"the reverse map of @ISA dependencies";
	root_["registered_mros"] = u8"the registered MROs HV";
	root_["rs"] = u8"the IRS";
	root_["last_in_gv"] = u8"the last input GV";
	root_["ofsgv"] = u8"the OFS GV";
	root_["defoutgv"] = u8"the default output GV";
	root_["hintgv"] = u8"the hints (%^H) GV";
	root_["patchlevel"] = u8"the patch level";
	root_["apiversion"] = u8"the API version";
	root_["e_script"] = u8"the '-e' script";
	root_["mess_sv"] = u8"the message SV";
	root_["ors_sv"] = u8"the ORS SV";
	root_["encoding"] = u8"the encoding";
	root_["blockhooks"] = u8"the block hooks";
	root_["custom_ops"] = u8"the custom ops HV";
	root_["custom_op_names"] = u8"the custom op names HV";
	root_["custom_op_descs"] = u8"the custom op descriptions HV";
}

