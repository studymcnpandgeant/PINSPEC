/*
 * Isotope.cpp
 *
 *  Created on: Mar 13, 2012
 *      Author: William Boyd
 *				MIT, Course 22
 *              wboyd@mit.edu
 */	

#include "Isotope.h"


/**
 * Isotope constructor sets default values for some isotope properties
 */
Isotope::Isotope(char* isotope_name){

	/* Default atomic number and number densities and temperature */
    _isotope_name = isotope_name;
	parseName();

	_T = 300;
	_kB = 8.617332E-5;             /* boltzmann's constant (ev / K) */
	_fissionable = false;

	loadXS(isotope_name, ELASTIC);
	loadXS(isotope_name, CAPTURE);

	/* By default this isotope has no cross-sections */
	_num_capture_xs = 0;
	_num_elastic_xs = 0;
	_num_fission_xs = 0;
	_num_absorb_xs = 0;
	_num_total_xs = 0;

	/* By default the thermal scattering cdfs have not been initialized */
	_num_thermal_cdfs = 0;
	_num_thermal_cdf_bins = 0;

	/* FIXME: may need to update these defaults later */
	initializeThermalScattering(1, 8, 20, 4);
}



/**
 * Isotope destructor deletes array of cross section values that
 * have been assigned to this isotope
 */
Isotope::~Isotope() {
	if (_num_elastic_xs != 0) {
		delete [] _elastic_xs;
		delete [] _elastic_xs_energies;
	}
	if (_num_absorb_xs != 0) {
		delete [] _absorb_xs;
		delete [] _absorb_xs_energies;
	}
	if (_num_capture_xs != 0) {
		delete [] _capture_xs;
		delete [] _capture_xs_energies;
	}
	if (_num_fission_xs != 0) {
		delete [] _fission_xs;
		delete [] _fission_xs_energies;
	}
	if (_num_total_xs != 0) {
		delete [] _total_xs;
		delete [] _total_xs_energies;
	}
	if (_num_thermal_cdfs != 0) {
		delete [] _thermal_dist;
		for (int i=0; i < _num_thermal_cdfs; i++)
			delete [] _thermal_cdfs[i];
		delete [] _thermal_cdfs;
		delete [] _E_to_kT;
		delete [] _Eprime_to_E;
	}
}


/**
 * Parse input name and set _A value for isotope
 */
void Isotope::parseName(){

	int A = 0;
	int i = 0;
	while (_isotope_name[i] != '\0'){

		/* if - encountered, get the A value */
		if (_isotope_name[i] == '-') {

			i++;
			A = atoi(&_isotope_name[i]);
			break;
		}

		i++;
	}

	/* Set the atomic number of isotope */
	setA(A);

	log_printf(DEBUG, "A value is: %i", _A);
}



/**
 * Returns the name of the of isotope
 * @return character array with name of isotope
 */
char* Isotope::getIsotopeType() const {
	return _isotope_name;
}


/**
 * Returns the atomic number of this isotope
 * @return the atomic number
 */
int Isotope::getA() const {
    return _A;
}


/**
 * Returns the alpha ((A-1)/(A+1))^2 values for this isotope
 * @return alpha
 */
float Isotope::getAlpha() const {
    return _alpha;
}


/**
 * Returns the number density for this isotope
 * @return the number density
 */
float Isotope::getN() const {
    return _N;
}


/**
 * Returns the relative atomic amount
 * @return the relative atomic amount
 */
float Isotope::getAO() const {
    return _AO;
}


/**
 * Return the temperature (Kelvin) for this isotope
 * @return the temperature of this isotope
 */
float Isotope::getTemperature() const {
	return _T;
}


/**
 * Return the average value of the cosine of theta for this isotope
 * in a scattering collision
 * @return the average for mu
 */
float Isotope::getMuAverage() const {
	return _mu_avg;
}


/**
 * Return whether this isotope is fissionable (true) or not (false)
 * @return whether this isotope is fissionable
 */
bool Isotope::isFissionable() const {
	return _fissionable;



}


/**
 * Returns an elastic scattering cross-section value for a certain
 * energy based on linear interpolation
 * @param energy the energy (eV) of interest
 * @return the elastic scattering cross-section (barns)
 */
float Isotope::getElasticXS(float energy) const {

	if (_num_elastic_xs == 0)
		return 0.0;

	/* Use linear interpolation to find the elastic scatter cross-section */
	return linearInterp<float, float, float>(_elastic_xs_energies,
						 _elastic_xs, _num_elastic_xs, 
						 energy);
}


/**
 * Returns an elastic cross-section value for a certain index into
 * the energy. This is meant to be used by a Material with rescaled
 * cross-sections
 * @param energy_index the index into the energy array
 * @return the elastic cross-section (barns)
 */
float Isotope::getElasticXS(int energy_index) const {

	if (_num_elastic_xs == 0)
		return 0.0;

	else if (energy_index > _num_elastic_xs) {
	    log_printf(ERROR, "Unable to retrieve elastic xs for"
		       " isotope %s since the energy index %d is out of"
		       " bounds", _isotope_name, energy_index);
	}

	return _elastic_xs[energy_index];
}


/**
 * Returns the type of angular elastic scattering distribution for
 * this isotope (ISOTROPIC_CM or ISOTROPIC_LAB)
 * @return the type of angular scattering distribution
 */
scatterAngleType Isotope::getElasticAngleType() const {

    if (_num_elastic_xs == 0) {
	log_printf(ERROR, "Cannot return an elastic angle type"
		   "for isotope %s since it has not been set", 
		   _isotope_name);
    }

	return _elastic_angle;
}


/**
 * Returns an absorption (capture plus fission) cross-section
 * value for a certain energy based on linear interpolation
 * @param energy the energy (eV) of interest
 * @return the absorption cross-section (barns)
 */
float Isotope::getAbsorptionXS(float energy) const {

	if (_num_absorb_xs == 0)
		return 0.0;

	/* Use linear interpolation to find the capture cross-section */
	return linearInterp<float, float, float>(_absorb_xs_energies,
								_absorb_xs, _num_absorb_xs, energy);
}


/**
 * Returns an absorption cross-section value for a certain index into
 * the energy. This is meant to be used by a Material with rescaled
 * cross-sections
 * @param energy_index the index into the energy array
 * @return the absorption cross-section (barns)
 */
float Isotope::getAbsorptionXS(int energy_index) const {

	if (_num_absorb_xs == 0)
		return 0.0;

	else if (energy_index > _num_absorb_xs) {
	    log_printf(ERROR, "Unable to retrieve absorption xs for"
		       " isotope %s since the energy index %d is out of"
		       " bounds", _isotope_name, energy_index);
	}
	return _absorb_xs[energy_index];
}


/**
 * Returns a capture cross-section value for a certain
 * energy based on linear interpolation
 * @param energy the energy (eV) of interest
 * @return the capture cross-section (barns)
 */
float Isotope::getCaptureXS(float energy) const{

	if (_num_capture_xs == 0)
		return 0.0;

	/* Use linear interpolation to find the capture cross-section */
	return linearInterp<float, float, float>(_capture_xs_energies,
								_capture_xs, _num_capture_xs, energy);
}


/**
 * Returns a capture cross-section value for a certain index into
 * the energy. This is meant to be used by a Material with rescaled
 * cross-sections
 * @param energy_index the index into the energy array
 * @return the capture cross-section (barns)
 */
float Isotope::getCaptureXS(int energy_index) const {

	if (_num_capture_xs == 0)
		return 0.0;

	else if (energy_index > _num_capture_xs)
		log_printf(ERROR, "Unable to retrieve capture xs for"
				" isotope %s since the energy index %d is out of"
				" bounds", _isotope_name, energy_index);

	return _capture_xs[energy_index];
}


/**
 * Returns a fission cross-section value for a certain
 * energy based on linear interpolation
 * @param energy the energy (eV) of interest
 * @return the fission cross-section (barns)
 */
float Isotope::getFissionXS(float energy) const{

	if (_num_fission_xs == 0)
		return 0.0;

	/* Use linear interpolation to find the fission cross-section */
	return linearInterp<float, float, float>(_fission_xs_energies,
							_fission_xs, _num_fission_xs, energy);
}


/**
 * Returns a fission cross-section value for a certain index into
 * the energy. This is meant to be used by a Material with rescaled
 * cross-sections
 * @param energy_index the index into the energy array
 * @return the fission cross-section (barns)
 */
float Isotope::getFissionXS(int energy_index) const {

	if (_num_fission_xs == 0)
		return 0.0;

	else if (energy_index > _num_fission_xs) {
	    log_printf(ERROR, "Unable to retrieve fission xs for"
		       " isotope %s since the energy index %d is out of"
		       " bounds", _isotope_name, energy_index);
	}

	return _fission_xs[energy_index];
}


/**
 * Returns a total scattering cross-section value for a certain
 * energy based on linear interpolation
 * @param energy the energy (eV) of interest
 * @return the total cross-section (barns)
 */
float Isotope::getTotalXS(float energy) const {

	/* If the total xs has been defined explicitly, use it to
	 * linearly interpolate to find the total cross-section */
	if (_num_total_xs != 0)
		return linearInterp<float, float, float>(_total_xs_energies,
									_total_xs, _num_total_xs, energy);

	/* Otherwise loop over all xs which have been defined and
	 * add them to a total xs */
	else {

		float total_xs = 0;

		total_xs += getAbsorptionXS(energy);
		total_xs += getElasticXS(energy);
		return total_xs;
	}
}


/**
 * Returns a total scattering cross-section value for a certain index
 * into the energy array
 * @param energy the energy (eV) of interest
 * @return the total cross-section (barns)
 */
float Isotope::getTotalXS(int energy_index) const {

	/* If the total xs has been defined explicitly, use it */
	if (_num_total_xs != 0) {

	    if (energy_index > _num_total_xs) {
		log_printf(ERROR, "Unable to retrieve total xs for"
			   " isotope %s since the energy index %d is out of"
			   " bounds", _isotope_name, energy_index);
	    }

		return _total_xs[energy_index];
	}

	/* Otherwise add absorption and elastic scatter xs to get a total xs */
	else {
		float total_xs = 0;
		total_xs += getAbsorptionXS(energy_index);
		total_xs += getElasticXS(energy_index);
		return total_xs;
	}
}


/**
 * Returns the transport cross-section for this isotope at a particular energy
 * @param energy the energy (eV) of interest
 * @return the transport cross-section (barns)
 */
float Isotope::getTransportXS(float energy) const {
	return (getTotalXS(energy) - _mu_avg * getElasticXS(energy));
}


/**
 * Returns the transport cross-section for this isotope at a particular index
 * into its energy array
 * @param energy_index the index into the energy array
 * @return the transport cross-section (barns)
 */
float Isotope::getTransportXS(int energy_index) const {
	return (getTotalXS(energy_index) - _mu_avg * getElasticXS(energy_index));
}


/**
 * This method returns true if the thermal scattering distributions
 * for this isotope have been initialized, and false otherwise
 * @return boolean if the thermal scattering distributions exist
 */
bool Isotope::usesThermalScattering() {

	if (_num_thermal_cdfs == 0)
		return false;
	else
		return true;
}


/**
 * Set the isotope name
 * @param istope a character array of the isotopes name
 */
void Isotope::setIsotopeType(char* isotope) {
	_isotope_name = isotope;
}


/**
 * Set the atomic number and update alpha, eta and rho
 * @param A atomic number
 */
void Isotope::setA(int A) {
    _A = A;
	_alpha = float(_A-1)/float(_A+1) * float(_A-1)/float(_A+1);
	_eta = (float(_A)+1.0) / (2.0 * sqrt(float(_A)));
	_rho = (float(_A)-1.0) / (2.0 * sqrt(float(_A)));
	_mu_avg = 2.0 / (3.0 * _A);
}


/**
 * Set the number density (at/cm^3)
 * @param N number density (at/cm^3)
 */
void Isotope::setN(float N) {
    _N = N;
}


/**
 * Set the relative atomic amount
 * @param AO relative atomic amount
 */
void Isotope::setAO(float AO) {
    _AO = AO;
}


/* Set the temperature (Kelvin)
 * @param T the temperature (Kelvin)
 */
void Isotope::setTemperature(float T) {
	_T = T;
}


/**
 * Make this a fissionable isotope
 */
void Isotope::makeFissionable() {
	_fissionable = true;
}


/**
 * Load a cross-section from an ASCII file into this isotope
 * @param filename the file with the cross-section values
 * @param type the type of cross-section
 * @param angle_type the type of angle (only used for scattering)
 * @param delimiter the character between data values in file
 */
void Isotope::loadXS(char* isotope_name, collisionType type) {

	std::string prefix = "../xs-lib/";
	std::string isotope = isotope_name;
	std::string filename;

	/* Set this isotope's appropriate cross-section using the data
	 * structures */
	if (type == ELASTIC){

		filename = prefix + isotope + "-elastic.txt";

		log_printf(INFO, "file name: %s", filename.c_str());

		/* Find the number of cross-section values in the file */
		int num_xs_values = getNumCrossSectionDataPoints(filename.c_str());

		/* Initialize data structures to store cross-section values */
		float* energies = new float[num_xs_values];
		float* xs_values = new float[num_xs_values];

		/* Parse the file into the data structures */
		parseCrossSections(filename.c_str(), energies, xs_values);

		setElasticXS(xs_values, energies, num_xs_values, ISOTROPIC_LAB);
	}
	else if (type == CAPTURE){

		filename = prefix + isotope + "-capture.txt";

		/* Find the number of cross-section values in the file */
		int num_xs_values = getNumCrossSectionDataPoints(filename.c_str());

		/* Initialize data structures to store cross-section values */
		float* energies = new float[num_xs_values];
		float* xs_values = new float[num_xs_values];

		/* Parse the file into the data structures */
		parseCrossSections(filename.c_str(), energies, xs_values);

		setCaptureXS(xs_values, energies, num_xs_values);
	}
	else if (type == FISSION) {

		filename = prefix + isotope + "-fission.txt";

		/* Find the number of cross-section values in the file */
		int num_xs_values = getNumCrossSectionDataPoints(filename.c_str());

		/* Initialize data structures to store cross-section values */
		float* energies = new float[num_xs_values];
		float* xs_values = new float[num_xs_values];

		/* Parse the file into the data structures */
		parseCrossSections(filename.c_str(), energies, xs_values);

		setFissionXS(xs_values, energies, num_xs_values);
	}

	return;
}


/**
 * Set the elastic cross-section for this isotope
 * @param elastic_xs a float array of microscopic elastic xs (barns)
 * @param elastic_xs_energies a float array of energies (eV)
 * @param num_elastic_xs the number of elastic xs values
 * @param type the type of angular scattering distribution
 */
void Isotope::setElasticXS(float* elastic_xs, float* elastic_xs_energies,
								int num_elastic_xs, scatterAngleType type) {
    _elastic_xs = elastic_xs;
    _elastic_xs_energies = elastic_xs_energies;
    _num_elastic_xs = num_elastic_xs;
    _elastic_angle = type;
    float (Isotope::*func)(float) const;
    func = &Isotope::getElasticXS;
    _xs_handles.insert(std::pair<collisionType, float(Isotope::*)(float)
    												const>(ELASTIC, func));
}


/**
 * Set the type of angular elastic scattering distribution for this isotope
 * @param type the angular elastic scattering distribution type
 */
void Isotope::setElasticAngleType(scatterAngleType type) {
	_elastic_angle = type;
}


/**
 * Set the capture cross-section for this isotope
 * @param capture_xs a float array of microscopic capture xs (barns)
 * @param capture_xs_energies a float array of energies (eV)
 * @param num_capture_xs the number of capture xs values
 */
void Isotope::setCaptureXS(float* capture_xs, float* capture_xs_energies,
													int num_capture_xs) {
    _capture_xs = capture_xs;
    _capture_xs_energies = capture_xs_energies;
    _num_capture_xs = num_capture_xs;
}


/**
 * Set the fission cross-section for this isotope
 * @param fission_xs a float array of microscopic fission xs (barns)
 * @param fission_xs_energies a float array of energies (eV)
 * @param num_fission_xs the number of fission xs values
 */
void Isotope::setFissionXS(float* fission_xs, float* fission_xs_energies,
													int num_fission_xs) {
    _fission_xs = fission_xs;
    _fission_xs_energies = fission_xs_energies;
    _num_fission_xs = num_fission_xs;
    float (Isotope::*func)(float) const;
    func = &Isotope::getFissionXS;
    _xs_handles.insert(std::pair<collisionType, float(Isotope::*)(float)
    											const>(FISSION, func));
}

/**
 * Rescales all cross-sections to a new energy grid by interpolating to find the
 * cross-section value at the old energy grid at the new points
 * @param energies the new energy grid
 * @param num_energies the number of points in the new energy grid
 */
void Isotope::rescaleXS(float* energies, int num_energies) {

	/* Capture xs */
	if (_num_capture_xs != 0) {
		float* new_energies = new float[num_energies];
		memcpy(new_energies, energies, sizeof(float)*num_energies);
		float* new_xs = new float[num_energies];

		for (int i=0; i < num_energies; i++)
			new_xs[i] = getCaptureXS(new_energies[i]);

		_num_capture_xs = num_energies;
		delete [] _capture_xs_energies;
		delete _capture_xs;
		setCaptureXS(new_xs, new_energies, num_energies);
		_capture_xs = new_xs;
		_capture_xs_energies = new_energies;
	}

	/* Elastic xs */
	if (_num_elastic_xs != 0) {
		float* new_energies = new float[num_energies];
		memcpy(new_energies, energies, sizeof(float)*num_energies);
		float* new_xs = new float[num_energies];

		for (int i=0; i < num_energies; i++)
			new_xs[i] = getElasticXS(new_energies[i]);

		_num_elastic_xs = num_energies;
		delete [] _elastic_xs_energies;
		delete _elastic_xs;
		_elastic_xs = new_xs;
		_elastic_xs_energies = new_energies;
	}

	/* Fission xs */
	if (_num_fission_xs != 0) {
		float* new_energies = new float[num_energies];
		memcpy(new_energies, energies, sizeof(float)*num_energies);
		float* new_xs = new float[num_energies];

		for (int i=0; i < num_energies; i++)
			new_xs[i] = getFissionXS(new_energies[i]);

		_num_fission_xs = num_energies;
		delete [] _fission_xs_energies;
		delete _fission_xs;
		setFissionXS(new_xs, new_energies, num_energies);
		_fission_xs = new_xs;
		_fission_xs_energies = new_energies;
	}

	log_printf(NORMAL, "generating absorption");

	/* Absorption xs */
	if (_num_absorb_xs != 0) {
		float* new_energies = new float[num_energies];
		memcpy(new_energies, energies, sizeof(float)*num_energies);
		float* new_xs = new float[num_energies];

		for (int i=0; i < num_energies; i++)
			new_xs[i] = getAbsorptionXS(new_energies[i]);

		_num_absorb_xs = num_energies;
		delete [] _absorb_xs_energies;
		delete _absorb_xs;
		_absorb_xs = new_xs;
		_absorb_xs_energies = new_energies;

	}

	/* Total xs */
	if (_num_capture_xs != 0) {
		float* new_energies = new float[num_energies];
		memcpy(new_energies, energies, sizeof(float)*num_energies);
		float* new_xs = new float[num_energies];

		for (int i=0; i < num_energies; i++)
			new_xs[i] = getTotalXS(new_energies[i]);

		_num_total_xs = num_energies;
		delete [] _total_xs_energies;
		delete _capture_xs;
		_total_xs = new_xs;
		_total_xs_energies = new_energies;
	}

	return;
}


/**
 * This method clones a given Isotope class object by executing a deep
 * copy of all of the Isotope's class attributes and giving them to a new
 * Isotope class object
 * @return a pointer to the new cloned Isotope class object
 */
Isotope* Isotope::clone() {

	/* Allocate memory for the clone */
	Isotope* new_clone = new Isotope(_isotope_name);

	/* Loops over all tallies and add them to the clone */
	for (std::vector<Tally*>::iterator it = _tallies.begin(); 
	     it != _tallies.end(); it++) {
	    new_clone->addTally(*it);
	}

	/* Set the clones isotope name, atomic number, number density */
	new_clone->setIsotopeType(_isotope_name);
	new_clone->setA(_A);
	new_clone->setN(_N);
	new_clone->setTemperature(_T);
	if (_fissionable)
	    new_clone->makeFissionable();

	/* If the given isotope has an elastic scatter xs */
	if (_num_elastic_xs > 0) {

		/* Deep copy the xs values */
		float* elastic_xs = new float[_num_elastic_xs];
		memcpy(elastic_xs, _elastic_xs, sizeof(float)*_num_elastic_xs);

		/* Deep copy the energies for each of the xs values */
		float* elastic_xs_energies = new float[_num_elastic_xs];
		memcpy(elastic_xs_energies, _elastic_xs_energies,
		       sizeof(float)*_num_elastic_xs);

		/* Set the clone's xs */
		new_clone->setElasticXS(elastic_xs, elastic_xs_energies,
				_num_elastic_xs, _elastic_angle);
	}

	/* If the capture xs has been generated for the given isotope */
	if (_num_capture_xs > 0) {

		/* Deep copy the xs values */
		float* capture_xs = new float[_num_capture_xs];
		memcpy(capture_xs, _capture_xs, sizeof(float)*_num_capture_xs);

		/* Deep copy the energies for each of the xs values */
		float* capture_xs_energies = new float[_num_capture_xs];
		memcpy(capture_xs_energies, _capture_xs_energies,
				sizeof(float)*_num_capture_xs);
	}

	/* If the given isotope has a fission xs */
	if (_num_fission_xs > 0) {

		/* Deep copy the xs values */
		float* fission_xs = new float[_num_fission_xs];
		memcpy(fission_xs, _fission_xs, sizeof(float)*_num_fission_xs);

		/* Deep copy the energies for each of the xs values */
		float* fission_xs_energies = new float[_num_fission_xs];
		memcpy(fission_xs_energies, _fission_xs_energies,
				sizeof(float)*_num_fission_xs);

		/* Set the clone's fission xs */
		new_clone->setFissionXS(fission_xs, fission_xs_energies,
													_num_fission_xs);
	}

	new_clone->rescaleXS(_capture_xs_energies, _num_capture_xs);

	/* Initialize the isotope's thermal scattering CDFs if they have been
	 * created for this isotope */
	if (_num_thermal_cdfs > 0) {
		new_clone->initializeThermalScattering
		    (_E_to_kT[0]*_kB*_T, _E_to_kT[_num_thermal_cdfs-1]*_kB*_T, 
		     _num_thermal_cdf_bins, _num_thermal_cdfs);
	}

	/* Return a pointer to the cloned Isotope class */
	return new_clone;
}


/**
 * For a given energy, this method determines a random collision type
 * based on the values of each of its cross-section types at that energy
 * @param energy the incoming neutron energy (eV)
 * @return the collision type (ELASTIC, CAPTURE, FISSION)
 */
collisionType Isotope::getCollisionType(float energy) {

	float test = float(rand()) / RAND_MAX;
	float collision_xs = 0.0;
	float next_collision_xs = 0.0;
	float total_xs = getTotalXS(energy);
	collisionType type;

	/* Loops over all cross-section types to find the one for this energy */
	std::map<collisionType, float(Isotope::*)(float)
												const>::const_iterator iter;

	for (iter = _xs_handles.begin(); iter != _xs_handles.end(); ++iter) {
		next_collision_xs += (this->*iter->second)(energy) / total_xs;

		if (test >= collision_xs && test <= next_collision_xs) {
			type = iter->first;
			break;
		}

		/* Update the next collision xs */
		collision_xs = next_collision_xs;
	}

	return type;
}

/**
 * For a given neutron energy in eV in a scattering collision, this
 * function returns the outgoing energy in eV, Eprime, for the collision
 * based on the thermal scattering distributions
 * @param energy the incoming energy (eV)
 * @return the outgoing energy (eV)
 */
float Isotope::getThermalScatteringEnergy(float energy) {

	/* First check that the thermal scattering CDFs have been initialized */
    if (_num_thermal_cdfs == 0) {
	log_printf(ERROR, "Unable to sample the thermal scattering CDFs for"
		   " isotope %s because they have not yet been initialized",
		   _isotope_name);
    }

	/* Convert energies in eV to eV / kT */
	energy /= (_kB * _T);

	/* Compute possible values for E to scatter to */
	float* possible_Eprimes = new float[_num_thermal_cdf_bins];
	for (int i=0; i < _num_thermal_cdf_bins; i++)
		possible_Eprimes[i] = _Eprime_to_E[i] * energy;

	float rn = float(rand()) / RAND_MAX;
	int index;
	float Eprime;

	/* Check if energy is lower than all thermal scattering CDFs */
	if (energy < _E_to_kT[0]) {
		index = findUpperIndex(_thermal_cdfs[0],
								_num_thermal_cdf_bins-1, 0, rn);
		Eprime = possible_Eprimes[index];
	}

	/* Check if energy is above all thermal scattering CDFs */
	else if (energy > _E_to_kT[_num_thermal_cdfs-1]) {
		index = findUpperIndex(_thermal_cdfs[_num_thermal_cdfs-1],
									_num_thermal_cdf_bins-1, 0, rn);
		Eprime = possible_Eprimes[index];
	}

	/* Otherwise the energy is sandwiched within the scattering CDFs */
	else {
		int upper_index = findUpperIndex(_E_to_kT, _num_thermal_cdfs-1,
															0, energy);
		int lower_index = upper_index - 1;
		int Eprime_lower_index = findUpperIndex(_thermal_cdfs[lower_index],
										_num_thermal_cdf_bins-1, 0, rn);
		float Eprime_lower = possible_Eprimes[Eprime_lower_index];
		int Eprime_upper_index = findUpperIndex(_thermal_cdfs[upper_index],
										_num_thermal_cdf_bins-1, 0, rn);
		float Eprime_upper = possible_Eprimes[Eprime_upper_index];
		float delta_E_to_kT = _E_to_kT[upper_index] - _E_to_kT[lower_index];
		float delta_Eprime = Eprime_upper - Eprime_lower;
		float slope = delta_Eprime / delta_E_to_kT;
		Eprime = slope * (energy - _E_to_kT[lower_index]) + Eprime_lower;
	}

	/* Convert outgoing energy back into eV */
	Eprime *= (_kB * _T);

	delete [] possible_Eprimes;

	return Eprime;
}


/* This method initializes the probability distributions for thermal
 * scattering. It takes in arguments for the start_energy and end
 * energy (ratios of kT) and the number of distributions which it
 * uses to generate logarithmically spaced energies for the distributions.
 * It also takes in the number of energy bins to use for each bin.
 * @param start_energy the first distribution's energy
 * @param end_energy the final distribution's energy
 * @param num_bins the number of bins per distribution
 * @param end_distributions the number of scattering distributions
 */
void Isotope::initializeThermalScattering(float start_energy,
					float end_energy, int num_bins, 
					  int num_distributions) {

	/* Number of thermal scattering distributions */
	_num_thermal_cdfs = num_distributions;

	/* Number of bins per distribution */
	_num_thermal_cdf_bins = num_bins;

	/* Allocate memory for distributions */
	_thermal_cdfs = new float*[_num_thermal_cdfs];
	for (int i=0; i < _num_thermal_cdfs; i++)
		_thermal_cdfs[i] = new float[_num_thermal_cdf_bins];

	_thermal_dist = new float[_num_thermal_cdfs * _num_thermal_cdf_bins];
	float* cdf = new float[_num_thermal_cdf_bins];

	/* Initialize logarithmically spaced E/kT for each distribution */
	_E_to_kT = logspace<float, float>(start_energy/(_kB*_T),
					  end_energy/(_kB*_T), 
					  _num_thermal_cdfs);

	/* Find the maximum Eprime / E value that we must extend our distributions
	 * to before they all fall below some tolerance */
	bool tolerance_met = false;
	float dist_tolerance = 0.1;
	float curr_prob;
	float curr_Eprime_to_E = 1.0;
	while (!tolerance_met) {

		/* Start with the tolerance being met */
		tolerance_met = true;

		/* Loop over all CDFs and check if we are within the threshold */
		for (int i=0; i < _num_thermal_cdfs; i++) {
			curr_prob = thermalScatteringProb(curr_Eprime_to_E, i);

			/* If we are above the tolerance */
			if (curr_prob > dist_tolerance)
				tolerance_met = false;
		}

		/* Update distance along x-axis */
		if(!tolerance_met)
			curr_Eprime_to_E += 0.25;
	}

	/* Initialize x-axis of Eprime to E ratios */
	_Eprime_to_E = logspace<float, float>(1E-5, curr_Eprime_to_E,
					      _num_thermal_cdf_bins);

	/* Loop over each distribution */
	for (int i=0; i < _num_thermal_cdfs; i++) {
		for (int j=0; j < _num_thermal_cdf_bins; j++)
			_thermal_dist[i*_num_thermal_cdf_bins + j] =
			    thermalScatteringProb(_Eprime_to_E[j], i);
	}

	/* Create CDFs for each distribution */
	for (int i=0; i < _num_thermal_cdfs; i++) {
		cumulativeIntegral(_Eprime_to_E,
				   &_thermal_dist[i*_num_thermal_cdf_bins], cdf,
				   _num_thermal_cdf_bins, TRAPEZOIDAL);

		/* Transfer CDF values to our array */
		for (int j=0; j < _num_thermal_cdf_bins; j++)
			_thermal_cdfs[i][j] = cdf[j];
	}    void rescaleXS(float* new_energies, int num_energies);


	delete [] cdf;

	/* Normalize CDFs */
	for (int i=0; i < _num_thermal_cdfs; i++) {
		for (int j=0; j < _num_thermal_cdf_bins; j++)
			_thermal_cdfs[i][j] /= _thermal_cdfs[i][_num_thermal_cdf_bins-1];
	}

	return;
}


/**
 * This function computes the thermal scattering probability for
 * for a ratio of initial to final energies
 * @param E_prime_to_E a ratio of initial to final energies
 * @return the probability of the ratio occurring
 */
float Isotope::thermalScatteringProb(float E_prime_to_E, int dist_index) {

	double prob;

    /* Computes the final energy for each of the ratios */
    float Eprime = _E_to_kT[dist_index] * E_prime_to_E;

    /* Uses the equation from 22.211 slide 26 of the 2nd lecture
     * to compute probabilities */
    double a = sqrt(_E_to_kT[dist_index]);
	double b = sqrt(Eprime);
	double c = erf(_eta * b - _rho * a);
	double d = erf(_eta * b + _rho * a);
	double e = erf(_eta * a - _rho * b);
	double f = erf(_eta * a + _rho * b);
	double g = exp(double(_E_to_kT[dist_index]) - double(Eprime));

	/* Account for lower and upper signs in equation */
	if (Eprime > _E_to_kT[dist_index])
		prob = (c - d) + g * (e + f);
	else
		prob = (c + d) + g * (e - f);

	/* Multiply by eta / 2 */
	prob *= double(_eta*_eta) / 2.0;

	/* Normalize to the atomic mass by multiplying by 1-alpha */
	prob *= (1.0 - double(_alpha));

	return float(prob);
}

void Isotope::addTally(Tally *tally) {
    tally->setTallyDomainType(ISOTOPE);
    _tallies.push_back(tally);
    return;
}


/**
 * Clear this isotope's vector of Tally class object pointers
 */
void Isotope::clearTallies() {
	_tallies.clear();
}

/**
 * For a given energy, this method calls getCollisionType() to sample 
 * the collision type, and then tally the event into the appropriate
 * tally classes for that isotope if any. 
 * @param neutron structure
 * @return the collision type (ELASTIC, CAPTURE, FISSION)
 */
collisionType Isotope::collideNeutron(neutron* neut) {
    /* obtain incoming energy, batch number */
    float energy = neut->_energy;
    int batch_num = neut->_batch_num;

    /* obtain collision type */
    collisionType type = getCollisionType(energy);
	if (type == FISSION || type == CAPTURE)
		neut->_alive = false;

    float total_xs = getTotalXS(energy);
    float elastic_xs = getElasticXS(energy);
    float absorption_xs = getAbsorptionXS(energy);
    float capture_xs = getCaptureXS(energy);
    float fission_xs = getFissionXS(energy);
    float transport_xs = getTransportXS(energy);

    /* FIXME: what is sample? */
    float sample = energy;

    /* Loops over all tallies and add them to the clone */
    /* FIXME: need cleanup/speedup  */
    std::vector<Tally*>::iterator iter;
	for (iter = _tallies.begin(); iter != _tallies.end(); iter ++) {
	    Tally *tally = *iter;
	    tallyType tally_type = tally->getTallyType();
	    switch (tally_type) {
	    case FLUX:
		tally->weightedTally(sample, 1.0 / total_xs, batch_num);
	    case COLLISION_RATE:
		    tally->weightedTally(sample, 1.0, batch_num);
	    case ELASTIC_RATE:
		if (type == ELASTIC)
		    tally->weightedTally(sample, elastic_xs / total_xs, 
					 batch_num);
	    case ABSORPTION_RATE:
		if (type == CAPTURE || type == FISSION)
		    tally->weightedTally(sample, absorption_xs / total_xs, 
					 batch_num);
	    case CAPTURE_RATE:
		if (type == CAPTURE)
		    tally->weightedTally(sample, capture_xs / total_xs, 
					 batch_num);
	    case FISSION_RATE:
		if (type == FISSION)
		    tally->weightedTally(sample, fission_xs / total_xs, 
					 batch_num);
	    case TRANSPORT_RATE:
		if (type == ELASTIC)
		    tally->weightedTally(sample, transport_xs / total_xs, 
					 batch_num);
	    case DIFFUSION_RATE: /* FIXME */
		if (type == ELASTIC)
		    tally->weightedTally(sample, 
					 1.0 / (3 * transport_xs * total_xs), 
					 batch_num); 
	    case LEAKAGE_RATE:; /* FIXME */
	    }
	}

	/* Sample outgoing energy uniformally between [alpha*E, E] */
	float alpha = getAlpha();
	double random = (float)(rand()) / (float)(RAND_MAX);
	neut->_energy = energy * (alpha + (1 - alpha) * random);

    return type;
}


/**
 * For a given neutron, this method samples a distance for the neutron.
 * @param neutron structure
 * @return the distance traveled
 */
float Isotope::getDistanceTraveled(neutron* neutron) {
    double Sigma, random, distance;

    Sigma = getTotalXS(neutron->_energy);
    random = (float)(rand()) / (float)(RAND_MAX);
    distance = - log(random)/Sigma;

    return distance;
}


/**
 * Generates a set of resonance integral bins based off user defined
 * range of energy bounds
 * @param array of energy values used to create RI tally bins
 * @param length of array
 */
void RIEnergies(float *ri_energies, int n){

	_ri_energies = new float [n];

	for (int i = 0; i < n; i++){
		_ri_energies[i] = ri_energies[i];
	}

	return;
}









