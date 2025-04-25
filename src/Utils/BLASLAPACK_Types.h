#ifndef BLASLAPACK_TYPES_H
#define BLASLAPACK_TYPES_H

#include <complex>

// This header ONLY defines the low-level BLAS/LAPACK types
// based on CMake preprocessor definitions. It should have
// minimal other includes to be safely included early.

// These preprocessor symbols (TPE_USE_MKL, TPE_USE_OPENBLAS, etc.)
// MUST be defined by the CMakeLists.txt based on TPE_BLAS_LAPACK_BACKEND choice.

// THIS FILE IS MANUALLY INDENTED. DO NOT FORMAT.

#ifdef TPE_USE_MKL
    #pragma message("BLASLAPACK_Types.h: Configuring for MKL backend.")

    // Ensure MKL_ILP64 definition is correct before including mkl.h
    #ifdef TPE_MKL_USE_ILP64
        #ifndef MKL_ILP64
            #define MKL_ILP64
        #endif
    #else
        #ifdef MKL_ILP64
            #undef MKL_ILP64
        #endif
    #endif
    #include <mkl.h> // Includes cblas and lapacke interfaces

    namespace Tensors_BLAS_Private {
        using Int           = MKL_INT;
        using Bool          = bool;
        using ComplexFloat  = MKL_Complex8;
        using ComplexDouble = MKL_Complex16;
        using CBLAS_ORDER   = ::CBLAS_ORDER; // Use global enum type from mkl_cblas.h
        constexpr CBLAS_ORDER CblasRowMajor = ::CblasRowMajor;
        constexpr CBLAS_ORDER CblasColMajor = ::CblasColMajor;
    }
    namespace Tensors_LAPACK_Private {
        using Int           = MKL_INT;
        using Bool          = bool;
        using ComplexFloat  = MKL_Complex8;
        using ComplexDouble = MKL_Complex16;
    }

#elif defined(TPE_USE_OPENBLAS)
    #pragma message("BLASLAPACK_Types.h: Configuring for OpenBLAS backend.")
    #include <cblas.h>
    #include <lapacke.h>

    #ifdef OPENBLAS_USE64BITINT
        using BlasLapackInt = long long;
    #else
        using BlasLapackInt = int;
    #endif

    namespace Tensors_BLAS_Private {
        using Int           = BlasLapackInt;
        using Bool          = bool;
        using ComplexFloat  = std::complex<float>;
        using ComplexDouble = std::complex<double>;
        using CBLAS_ORDER   = ::CBLAS_ORDER;
        constexpr CBLAS_ORDER CblasRowMajor = ::CblasRowMajor;
        constexpr CBLAS_ORDER CblasColMajor = ::CblasColMajor;
    }
    namespace Tensors_LAPACK_Private {
        using Int           = BlasLapackInt;
        using Bool          = bool;
        using ComplexFloat  = lapack_complex_float;  // From lapacke.h
        using ComplexDouble = lapack_complex_double; // From lapacke.h
    }

#elif defined(TPE_USE_ACCELERATE)
    #pragma message("BLASLAPACK_Types.h: Configuring for Accelerate backend.")
    #ifndef ACCELERATE_NEW_LAPACK
        #define ACCELERATE_NEW_LAPACK
    #endif
    #include <Accelerate/Accelerate.h>

    using BlasLapackInt = __CLPK_integer;

    namespace Tensors_BLAS_Private {
        using Int           = BlasLapackInt;
        using Bool          = bool;
        using ComplexFloat  = __CLPK_complex;
        using ComplexDouble = __CLPK_doublecomplex;
        using CBLAS_ORDER   = ::CBLAS_ORDER;
        constexpr CBLAS_ORDER CblasRowMajor = ::CblasRowMajor;
        constexpr CBLAS_ORDER CblasColMajor = ::CblasColMajor;
    }
    namespace Tensors_LAPACK_Private {
        using Int           = BlasLapackInt;
        using Bool          = bool;
        using ComplexFloat  = __CLPK_complex;
        using ComplexDouble = __CLPK_doublecomplex;
    }

#else
    #error "No BLAS/LAPACK backend defined! Set TPE_BLAS_LAPACK_BACKEND in CMake."
#endif


// --- Public Type Aliases ---
namespace Tensors {
    namespace BLAS {
        using Int           = Tensors_BLAS_Private::Int;
        using Bool          = Tensors_BLAS_Private::Bool;
        using ComplexFloat  = Tensors_BLAS_Private::ComplexFloat;
        using ComplexDouble = Tensors_BLAS_Private::ComplexDouble;
        using ::CBLAS_ORDER;
        using Tensors_BLAS_Private::CblasRowMajor;
        using Tensors_BLAS_Private::CblasColMajor;
    }
    namespace LAPACK {
        using Int           = Tensors_LAPACK_Private::Int;
        using Bool          = Tensors_LAPACK_Private::Bool;
        using ComplexFloat  = Tensors_LAPACK_Private::ComplexFloat;
        using ComplexDouble = Tensors_LAPACK_Private::ComplexDouble;
    }

    #ifdef TPE_USE_MKL
        constexpr bool AppleAccelerateQ = false;
        constexpr bool OpenBLASQ        = false;
        constexpr bool MKLQ             = true;
    #elif defined(TPE_USE_OPENBLAS)
        constexpr bool AppleAccelerateQ = false;
        constexpr bool OpenBLASQ        = true;
        constexpr bool MKLQ             = false;
    #elif defined(TPE_USE_ACCELERATE)
        constexpr bool AppleAccelerateQ = true;
        constexpr bool OpenBLASQ        = false;
        constexpr bool MKLQ             = false;
    #else
        constexpr bool AppleAccelerateQ = false;
        constexpr bool OpenBLASQ        = false;
        constexpr bool MKLQ             = false;
    #endif

} // namespace Tensors


#endif // BLASLAPACK_TYPES_H