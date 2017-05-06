#include <cstdio>
#include <string>

#define LIN64
extern "C" {

	#include <base/io/ioAbc.h>
	void Abc_Start();
	void Abc_Stop();
	void * Abc_FrameGetGlobalFrame();
	int Cmd_CommandExecute( void * pAbc, char * sCommand );
	struct Abc_Frame_t;
	Abc_Ntk_t* Abc_FrameReadNtk(Abc_Frame_t* p);
}

using namespace std;

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

int main(int argc, char** argv) {
	printf("Hello IWLS'17!\n");

	if (argc != 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return 1;
	}

	string infile(argv[1]);
	const auto max_tries = 3;

	Abc_Start();
	auto pAbc = Abc_FrameGetGlobalFrame();

	try_command(pAbc, "read_library ygates.genlib");

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
		improvement = impnnodes < innodes;
	} while (improvement);

	try_command(pAbc, choice);
	try_command(pAbc, "map -a; print_stats");
	try_command(pAbc, "map -a; print_stats");
	try_command(pAbc, "map -a; print_stats");

	try_command(pAbc, "write_verilog " + infile + "_opt.v");

	Abc_Stop();

	return 0;
}

