
#include <vector>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QFileDialog>
#include <math.h>
#include <stdlib.h>
#include "app.h"
#include "standard_tools.h"
#include "tooltips.h"

inline unsigned myatoi(std::string line, unsigned& v, char end=0){
	unsigned idx = 0;
	v = line[idx]-'0';
	++idx;
	while(line[idx]!=end && idx < line.size()){
		v *= 10;
		v += line[idx]-'0';
		++idx;
	}
	return idx;
}

class UG_LogParser{
public:
	UG_LogParser(std::string &filename){
		std::cerr << "UG parser filename: " << filename << std::endl;
		_is = new std::ifstream(filename);
	}

	~UG_LogParser(){
		delete _is;
	}

	std::string get_line(){
		getline(*_is, _line);

		if(_is->eof()){
			return std::string();
		}
		else{
			skip_initial_spaces();
			remove_stupid_windoof_eol_if();
			return _line;
		}
	}

	void skip_initial_spaces(){
		unsigned i = 0;
		while(_line.size() && _line[i] == ' '){
			++i;
		}
		_line = _line.substr(i, _line.size());
	}

	void remove_initial_spaces(std::string &s){
		unsigned i = 0;
		while(s.size() && s[i] == ' '){
			++i;
		}
		s = s.substr(i, s.size());
	}

	void remove_until(std::string &s, char token){
		unsigned i = 0;
		while(s.size() && s[i] != token){
			++i;
		}
		s = s.substr(i+1, s.size());
	}

	void remove_stupid_windoof_eol_if(){
		if(_line.size() && int(_line[_line.size()-1] == 13)){
			_line = _line.substr(0, _line.size()-1);
		}
	}

	bool is_contained(std::string token){
		if(!_line.size()){
			get_line();
		}
		if(_line.find(token) == std::string::npos){
			return false;
		}
		return true;
	}

	void skip_until(std::string token){
		while(_line.find(token) == std::string::npos){
			get_line();
		}
	}

	void skip_until2(std::string token1, std::string token2){
		while(_line.find(token1) == std::string::npos && _line.find(token2) == std::string::npos){
			get_line();
		}
	}

	std::string get_value(std::string &line, std::string token, char delim='='){
		unsigned pos = line.find(token);
		pos = line.find(delim, pos);
		unsigned pos2 = line.find("\n", pos+1);
		return line.substr(pos+2, pos2-pos-1);
	}

	std::string get_value(std::string &line, std::string token, std::string end){
		unsigned pos = line.find(token);
		unsigned pos2 = line.find(end, pos);
		return line.substr(pos+token.size(), pos2-pos-token.size());
	}

	void parse_general_parameters(){
		skip_until("General parameters chosen");
		get_line();

		std::string s_grid = get_value(_line, "grid");
		get_line();
		std::string s_numRefs = get_value(_line, "numRefs");
		get_line();
		std::string s_numPreRefs = get_value(_line, "numPreRefs");
		get_line();
		std::string s_numProcs = get_value(_line, "numProcs");
		get_line();
		std::string s_material = get_value(_line, "material");
		get_line();
		std::string s_evIterations = get_value(_line, "evIterations");
		get_line();
		std::string s_evPrec = get_value(_line, "evPrec");

		myatoi(s_numRefs, _numRefs);
		myatoi(s_numPreRefs, _numPreRefs);
		myatoi(s_numProcs, _numProcs);
		_material = s_material;

		std::cout.precision(s_evPrec.size());
		_evPrec = std::stod(s_evPrec);
	}

	void parse_solver_parameters(){
 		skip_until("Number of EV");
		std::string s_numevs = get_value(_line, "Number of EV");
		myatoi(s_numevs, _numevs);

		get_line();
		std::string s_pinvit = get_value(_line, "PINVIT = ");

		_pinvit = s_pinvit;

		get_line();
		get_line();

		std::string s_preconditioner;

		if(is_contained("GeometricMultigrid")){
			s_preconditioner = "GMG";
		}
		if(is_contained("V-Cycle")){
			s_preconditioner += "(V-Cycle)";
		}

		_preconditioner = s_preconditioner;

		get_line();

		std::string s_smoother = _line;

		_smoother = s_smoother;

		get_line();

		std::string s_baselevel = get_value(_line, "Baselevel = ", ", ");
		myatoi(s_baselevel, _baselevel);

		get_line();

		std::string s_basesolver;
		if(is_contained("LU Decomposition")){
			s_basesolver = "LU Decomposition";
		}

		get_line();
		get_line();
		get_line();

		std::string s_additional_evs = _line;
		_additional_evs = s_additional_evs;
	}

	void parse_iterations(){
		skip_until("iteration");
		get_line();

		unsigned iter = 0;
		while(true){
			_lambdas.push_back(std::vector<double>(_numevs));
			_defects.push_back(std::vector<double>(_numevs));
			for(unsigned ev = 0; ev < _numevs; ++ev){
				std::string ss = get_value(_line, "defect: ", "reduction:");
				std::string dd = get_value(_line, "lambda: ", "defect:");
				remove_initial_spaces(ss);
				remove_initial_spaces(dd);
				std::cout.precision(ss.size());
				double d = std::stod(ss);
				std::cout.precision(dd.size());
				double d2 = std::stod(dd);

				_lambdas[iter][ev] = d;
				_defects[iter][ev] = d2;

				get_line();
			}
			++iter;
			skip_until2("iteration", "Eigenvalue");
			if(is_contained("Eigenvalue")){
				return;
			}
			get_line();
		}
	
	}

	void parse_solution_frequencies(){
		get_line();
		skip_until("Eigenvalue");

		for(unsigned ev = 0; ev < _numevs; ++ev){
			remove_until(_line, '=');
			remove_until(_line, '=');
			std::string f = _line.substr(1, _line.size()-3);

			std::cout.precision(f.size());
			_frequencies.push_back(std::stod(f));
			get_line();
		}
	}

	void parse_time(){
		skip_until("duration");
		std::string s_assembly = get_value(_line, "duration assembly", ':');
		s_assembly = s_assembly.substr(0, s_assembly.size()-1);

		std::cout.precision(s_assembly.size());
		_time_assembly = std::stod(s_assembly);

		get_line();

		std::string s_solver = get_value(_line, "duration solver", ':');
		s_solver = s_solver.substr(0, s_solver.size()-1);

		std::cout.precision(s_solver.size());
		_time_solver = std::stod(s_solver);

		get_line();

		std::string s_total = get_value(_line, "duration total", ':');
		s_total = s_total.substr(0, s_total.size()-1);
		
		std::cout.precision(s_total.size());
		_time_total = std::stod(s_total);
	}

	void do_it(){
		parse_general_parameters();
		parse_solver_parameters();
		parse_iterations();
		parse_solution_frequencies();
		parse_time();
	}

	unsigned num_evs(){
		return _numevs;
	}

	unsigned num_iterations(){
		return _lambdas.size();
	}

	unsigned num_refs(){
		return _numRefs;
	}

	unsigned num_prerefs(){
		return _numPreRefs;
	}

	unsigned num_procs(){
		return _numProcs;
	}

	unsigned num_max_iterations(){
		return _evIterations;
	}

	std::string material(){
		return _material;
	}

	double ev_precision(){
		return _evPrec;
	}

	unsigned baselevel(){
		return _baselevel;
	}

	double time_assembly(){
		return _time_assembly;
	}

	double time_solver(){
		return _time_solver;
	}

	double time_total(){
		return _time_total;
	}

	std::string pinvit(){
		return _pinvit;
	}

	std::string smoother(){
		return _smoother;
	}

	std::string preconditioner(){
		return _preconditioner;
	}

	std::string basesolver(){
		return _basesolver;
	}

	std::string additional_evs(){
		return _additional_evs;
	}

	void lambdas(std::vector<std::vector<double> > &lambdas){
		lambdas = _lambdas;
	}

	void defects(std::vector<std::vector<double> > &defects){
		defects = _defects;
	}

	void frequencies(std::vector<double> &freqs){
		freqs = _frequencies;
	}

public:
	std::ifstream* _is;
	std::string _line;

	//General parameters choosen
    unsigned _numRefs;
    unsigned _numPreRefs;
    unsigned _numProcs;
    std::string _material;
  	unsigned _evIterations;
  	double _evPrec;
	unsigned _numevs;

	//Solver parameters
	std::string _pinvit;
	std::string _smoother;
	std::string _preconditioner;
	unsigned _baselevel;
	std::string _basesolver;
	std::string _additional_evs;

	//Iteration info
	std::vector<std::vector<double> > _lambdas;
	std::vector<std::vector<double> > _defects;
	std::vector<double> _frequencies;

	//Time info
	double _time_assembly;
	double _time_solver;
	double _time_total;
};

