/***************************************************************************
 *  Copyright 2015 Marcelo Y. Matuda                                       *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#ifndef FFTW_H_
#define FFTW_H_

#include <mutex>

#include "fftw3.h"

#include "Exception.h"



namespace GS {

struct FFTWException : std::runtime_error {
	using std::runtime_error::runtime_error;
};

// The default plan flag is FFTW_MEASURE, with which deterministic results are
// obtained during an execution, but if the program is stopped and started,
// the results will be slightly different between executions.
// To produce deterministic results with FFTW_MEASURE, the wisdom must be saved
// at the end of the execution and restored at the start, using the functions:
//
// fftw_export_wisdom_to_filename();
// fftwf_export_wisdom_to_filename();
//
// fftwf_import_wisdom_from_filename();
// fftw_import_wisdom_from_filename();
//
// With the flag FFTW_ESTIMATE the results are always deterministic, but the
// execution is slower.

struct FFTWPlan {
	enum Type {
		TYPE_INVALID = 0,
		TYPE_FLOAT   = 1,
		TYPE_DOUBLE  = 2
	};

	Type type;
	//union {
		fftwf_plan planF;
		//fftw_plan plan;
	//};

	FFTWPlan() : type(TYPE_INVALID), planF() { }

	void reset() {
		type = TYPE_INVALID;
		planF = nullptr;
	}
};

// Plan execution is thread-safe.
// Plan creation and destruction must use an instance of this class.
// The constructor locks the mutex and the destructor unlocks it.
class FFTW {
public:
	enum Component {
		REAL = 0,
		IMAG = 1
	};

	FFTW();
	~FFTW();

	FFTWPlan plan_dft_r2c_1d(int n, float* in, fftwf_complex* out, unsigned int flags = 0) {
		fftwf_plan p = fftwf_plan_dft_r2c_1d(n, in, out, flags);
		if (p == nullptr) THROW_EXCEPTION(FFTWException, "Error in fftw_plan_dft_r2c_1d (float).");
		FFTWPlan plan;
		plan.type = FFTWPlan::TYPE_FLOAT;
		plan.planF = p;
		return plan;
	}

	FFTWPlan plan_idft_c2r_1d(int n, fftwf_complex* in, float* out, unsigned int flags = 0) {
		fftwf_plan p = fftwf_plan_dft_c2r_1d(n, in, out, flags);
		if (p == nullptr) THROW_EXCEPTION(FFTWException, "Error in fftw_plan_dft_c2r_1d (float).");
		FFTWPlan plan;
		plan.type = FFTWPlan::TYPE_FLOAT;
		plan.planF = p;
		return plan;
	}

	FFTWPlan plan_dft_1d(int n, fftwf_complex* in, fftwf_complex* out, unsigned int flags = 0) {
		fftwf_plan p = fftwf_plan_dft_1d(n, in, out, FFTW_FORWARD, flags);
		if (p == nullptr) THROW_EXCEPTION(FFTWException, "Error in fftw_plan_dft_1d (float, forward).");
		FFTWPlan plan;
		plan.type = FFTWPlan::TYPE_FLOAT;
		plan.planF = p;
		return plan;
	}

	FFTWPlan plan_idft_1d(int n, fftwf_complex* in, fftwf_complex* out, unsigned int flags = 0) {
		fftwf_plan p = fftwf_plan_dft_1d(n, in, out, FFTW_BACKWARD, flags);
		if (p == nullptr) THROW_EXCEPTION(FFTWException, "Error in fftw_plan_dft_1d (float, backward).");
		FFTWPlan plan;
		plan.type = FFTWPlan::TYPE_FLOAT;
		plan.planF = p;
		return plan;
	}

	void destroy_plan(FFTWPlan& p) {
		switch (p.type) {
		case FFTWPlan::TYPE_FLOAT:
			fftwf_destroy_plan(p.planF);
			break;
//		case FFTWPlan::TYPE_DOUBLE:
//			fftw_destroy_plan(p.plan);
//			break;
		default:
			return;
		}
		p.reset();
	}

	template<typename T> static T* alloc_real(size_t n);
	template<typename T> static T (*alloc_complex(size_t n))[2]; // returns pointer to array of size 2

	static void free(fftwf_complex* p) {
		fftwf_free(p);
	}
	static void free(float* p) {
		fftwf_free(p);
	}

	static void execute(FFTWPlan p) {
		switch (p.type) {
		case FFTWPlan::TYPE_FLOAT:
			fftwf_execute(p.planF);
			break;
//		case FFTWPlan::TYPE_DOUBLE:
//			fftw_execute(p.plan);
//			break;
		default:
			THROW_EXCEPTION(InvalidValueException, "Invalid plan type: " << p.type << '.');
		}
	}
private:
	FFTW(const FFTW&) = delete;
	FFTW& operator=(const FFTW&) = delete;
	FFTW(FFTW&&) = delete;
	FFTW& operator=(FFTW&&) = delete;

	static std::mutex mutex_;
};

} /* namespace GS */

#endif /* FFTW_H_ */
