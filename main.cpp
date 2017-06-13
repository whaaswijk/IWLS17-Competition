#include <cstdio>
#include <string>
#include <ctime>
#include <unordered_map>

#define LIN64
extern "C" {
	#include <base/io/ioAbc.h>
	void Abc_Start();
	void Abc_Stop();
	void * Abc_FrameGetGlobalFrame();
	int Cmd_CommandExecute( void * pAbc, char * sCommand );
	struct Abc_Frame_t;
	Abc_Ntk_t* Abc_FrameReadNtk(Abc_Frame_t* p);
	#include <mlp.h>
}
#include <xmg.h>
#include <logic_rewriting.h>

using namespace std;
using namespace majesty;

const string resyn = "balance; rewrite; rewrite -z; balance; rewrite -z; balance";
const string resyn2 = "balance; rewrite; refactor; balance; rewrite; rewrite -z; balance; refactor -z; rewrite -z; balance";
const string resyn3 = "balance; resub; resub -K 6; balance; resub -z; resub -z -K 6; balance; resub -z -K 5; balance";
const string compress = "balance -l; rewrite -l; rewrite -z -l; balance -l; rewrite -z -l; balance -l";
const string compress2 = "balance -l; rewrite -l; refactor -l; balance -l; rewrite -l; rewrite -z -l; balance -l; refactor -z -l; rewrite -z -l; balance -l";
const string choice = "fraig_store; " + resyn + "; fraig_store; " + resyn2 + "; fraig_store; fraig_restore";

void try_command(void* pAbc, const string& scmd) {
	static char cmd[256];
	char *ccmd = (char*)scmd.c_str();	
	sprintf(cmd, "%s", ccmd);
	if (Cmd_CommandExecute(pAbc, cmd)) {
		printf("Error executing command \"%s\".\n", cmd);
		exit(1);
	}
}

inline Abc_Ntk_t* get_ntk(void* pAbc) {
	Abc_FrameReadNtk((Abc_Frame_t*)pAbc);
}

string node_name(const node& n, nodeid idx, bool c, const unordered_map<nodeid,nodeid>& wiremap) {
	string res = c ? "~" : "";
	if (idx == 0) {
		if (c) {
			res = "0";
		} else {
			res = "1";
		}
	} else if (is_pi(n)) {
		res += string("i") + to_string(idx);
	} else {
		res += string("w") + to_string(wiremap.at(idx));
	}
	return res;
}

void xmg_to_yigfile(const xmg& m, string filename) {
	auto fhandle = fopen(filename.c_str(), "w");

	fprintf(fhandle, ".i %u\n", m.nin());
	fprintf(fhandle, ".o %u\n", m.nout());

	const auto& nodes = m.nodes();
	const auto& outputs = m.outputs();
	const auto& outcompl = m.outcompl();
	
	unordered_map<nodeid,nodeid> wiremap;
	nodeid widx = 1;
	for (auto i = 0u; i < nodes.size(); i++) {
		const auto& node = nodes[i];
		if (is_pi(node)) {
			continue;
		}
		wiremap[i] = widx++;
	}
	fprintf(fhandle, ".w %u\n", widx-1);

	for (auto i = 0u; i < nodes.size(); i++) {
		const auto& node = nodes[i];
		if (is_pi(node)) {
			continue;
		}
		auto name = "w" + to_string(wiremap[i]);
		auto inname1 = node_name(nodes[node.in1], node.in1, is_c1(node), wiremap);
		auto inname2 = node_name(nodes[node.in2], node.in2, is_c2(node), wiremap);
		auto inname3 = node_name(nodes[node.in3], node.in3, is_c3(node), wiremap);
		fprintf(fhandle, "%s = Y1(%s, %s, %s)\n", name.c_str(), inname1.c_str(), inname2.c_str(), inname3.c_str());
	}

	for (auto i = 0; i < outputs.size(); i++) {
		const auto nodeid = outputs[i];
		auto name = "o" + to_string(i+1);
		auto wnode = nodes[nodeid];
		auto wname = node_name(wnode, nodeid, outcompl[i], wiremap);

		if (nodeid == 0) {
			if (outcompl[i]) {
				fprintf(fhandle, "%s = Y0(0)\n", name.c_str());
			} else {
				fprintf(fhandle, "%s = Y0(1)\n", name.c_str());
			}
		} else {
			fprintf(fhandle, "%s = Y0(%s)\n", name.c_str(), wname.c_str());
		}
	}

	fprintf(fhandle, ".e\n");

	fclose(fhandle);
}

int main(int argc, char** argv) {
	clock_t begin = clock();

	if (argc != 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return 1;
	}

	string infile(argv[1]);
	const auto max_tries = 1;

	Abc_Start();
	auto pAbc = Abc_FrameGetGlobalFrame();


	try_command(pAbc, "alias b balance");
	try_command(pAbc, "alias st strash");
	try_command(pAbc, "alias rs resub");
	try_command(pAbc, "alias rw rewrite");
	try_command(pAbc, "alias rwz rewrite -z");
	try_command(pAbc, "alias rf refactor");
	try_command(pAbc, "alias rfz refactor -z");
	try_command(pAbc, "alias resyn \"b; rw; rwz; b; rwz; b\"");
	try_command(pAbc, "alias resyn2 \"b; rw; rf; b; rw; rwz; b; rfz; rwz; b\"");
	try_command(pAbc, "alias src_rs \"strash; resub -K 6 -N 2 -l; resub -K 9 -N 2 -l; resub -K 12 -N 2 -l\"");
	try_command(pAbc, "alias src_rws     \"st; rw -l; rs -K 6 -N 2 -l; rwz -l; rs -K 9 -N 2 -l; rwz -l; rs -K 12 -N 2 -l\"");
	try_command(pAbc, "alias resyn2rs    \"b; rs -K 6; rw; rs -K 6 -N 2; rf; rs -K 8; b; rs -K 8 -N 2; rw; rs -K 10; rwz; rs -K 10 -N 2; b; rs -K 12; rfz; rs -K 12 -N 2; rwz; b\"");
	try_command(pAbc, "alias compress2rs \"b -l; rs -K 6 -l; rw -l; rs -K 6 -N 2 -l; rf -l; rs -K 8 -l; b -l; rs -K 8 -N 2 -l; rw -l; rs -K 10 -l; rwz -l; rs -K 10 -N 2 -l; b -l; rs -K 12 -l; rfz -l; rs -K 12 -N 2 -l; rwz -l; b -l\"");
	try_command(pAbc, "alias drwsat2 \"st; drw; b -l; drw; drf; ifraig -C 20; drw; b -l; drw; drf\"");
	try_command(pAbc, "alias choice2 \"fraig_store; balance; fraig_store; resyn; fraig_store; resyn2; fraig_store; resyn2; fraig_store; fraig_restore\"");

	printf("Reading %s\n", infile.c_str());
	try_command(pAbc, "read_verilog " + infile);
	try_command(pAbc, "strash");
	try_command(pAbc, "print_stats");
	auto tries = 0;
	auto improvement = false;
	do {
		auto ntk = get_ntk(pAbc);
		auto innodes = Abc_NtkNodeNum(ntk);
		auto cnnodes = innodes;
		auto impnnodes = innodes;
		tries = 0;
		while (true) {
			try_command(pAbc, resyn3);
			try_command(pAbc, "print_stats");
			ntk = get_ntk(pAbc);
			impnnodes = Abc_NtkNodeNum(ntk);
			tries++;
			if (impnnodes < cnnodes) {
				tries = 0;
				cnnodes = impnnodes;
				continue;
			} else if (tries < max_tries) {
				continue;
			} else {
				break;
			}
		}
		tries = 0;
		while (true) {
			try_command(pAbc, resyn2);
			try_command(pAbc, "print_stats");
			ntk = get_ntk(pAbc);
			impnnodes = Abc_NtkNodeNum(ntk);
			tries++;
			if (impnnodes < cnnodes) {
				tries = 0;
				cnnodes = impnnodes;
			} else if (tries < max_tries) {
				continue;
			} else {
				break;
			}
		}

		// Compress
		tries = 0;
		while (true) {
			try_command(pAbc, compress);
			try_command(pAbc, "print_stats");
			ntk = get_ntk(pAbc);
			impnnodes = Abc_NtkNodeNum(ntk);
			tries++;
			if (impnnodes < cnnodes) {
				tries = 0;
				cnnodes = impnnodes;
			} else if (tries < max_tries) {
				continue;
			} else {
				break;
			}
		}

		// Compress2
		tries = 0;
		while (true) {
			try_command(pAbc, compress2);
			try_command(pAbc, "print_stats");
			ntk = get_ntk(pAbc);
			impnnodes = Abc_NtkNodeNum(ntk);
			tries++;
			if (impnnodes < cnnodes) {
				tries = 0;
				cnnodes = impnnodes;
			} else if (tries < max_tries) {
				continue;
			} else {
				break;
			}
		}
		try_command(pAbc, "drwsat2");
		try_command(pAbc, "src_rs");
		try_command(pAbc, "src_rws");
		try_command(pAbc, "resyn2rs");
		try_command(pAbc, "compress2rs");
		try_command(pAbc, "choice2");
		try_command(pAbc, resyn3);
		improvement = impnnodes < innodes;
	} while (improvement);
	try_command(pAbc, "strash");
	try_command(pAbc, "short_names");

	auto abc_opt_filename = infile + "_opt.aig";
	try_command(pAbc, "write_aiger " + abc_opt_filename);

	Abc_Stop();

	LogicCircuit* lc = NULL;
	{
		auto fhandle = fopen(abc_opt_filename.c_str(), "r");
		lc = readaiger(fhandle);
		fclose(fhandle);
	}
	auto mig = cloneLC(lc);
	xmg m(mig);
	assert(m.is_mig());
	freemig(mig);
	freecircuit(lc);
	// ABC naming bug
	for (auto i = 0u; i < m.nin(); i++) {
		m.set_inname(i, "i" + to_string(i+1));
	}
	for (auto i = 0u; i < m.nout(); i++) {
		m.set_outname(i, "o" + to_string(i+1));
	}


	//	auto p = default_xmg_params();
	//	auto opt_xmg = lut_area_strategy(m, p.get(), 4);

	xmg_to_yigfile(m, infile + ".yig");

	clock_t end = clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
	printf("Total elapsed time is %f seconds\n", elapsed_secs);
		
	return 0;
}

