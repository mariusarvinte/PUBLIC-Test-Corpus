#include "clar_libgit2.h"

void test_core_features__basic(void)
{
	int caps = git_libgit2_features();

#if defined(GIT_NSEC)
	cl_assert((caps & GIT_FEATURE_NSEC) != 0);
#else
	cl_assert((caps & GIT_FEATURE_NSEC) == 0);
#endif

	cl_assert((caps & GIT_FEATURE_REGEX) != 0);

	cl_assert((caps & GIT_FEATURE_COMPRESSION) != 0);
	cl_assert((caps & GIT_FEATURE_SHA1) != 0);


	/*
	 * Ensure that our tests understand all the features;
	 * this test tries to ensure that if there's a new feature
	 * added that the backends test (below) is updated as well.
	 */
	cl_assert((caps & ~(GIT_FEATURE_NSEC |
	                    GIT_FEATURE_REGEX |
	                    GIT_FEATURE_COMPRESSION |
	                    GIT_FEATURE_SHA1 |
	                    GIT_FEATURE_SHA256
			    )) == 0);
}

void test_core_features__backends(void)
{
	const char *nsec = git_libgit2_feature_backend(GIT_FEATURE_NSEC);
	const char *regex = git_libgit2_feature_backend(GIT_FEATURE_REGEX);
	const char *compression = git_libgit2_feature_backend(GIT_FEATURE_COMPRESSION);
	const char *sha1 = git_libgit2_feature_backend(GIT_FEATURE_SHA1);
	const char *sha256 = git_libgit2_feature_backend(GIT_FEATURE_SHA256);

#if defined(GIT_NSEC)
	cl_assert_equal_s("mtim", nsec);
#else
	cl_assert(nsec == NULL);
#endif

#if defined(GIT_REGEX_PCRE2)
	cl_assert_equal_s("pcre2", regex);
#else
	cl_assert(0);
#endif

#if defined(GIT_COMPRESSION_BUILTIN)
	cl_assert_equal_s("builtin", compression);
#elif defined(GIT_COMPRESSION_ZLIB)
	cl_assert_equal_s("zlib", compression);
#else
	cl_assert(0);
#endif

#if defined(GIT_SHA1_BUILTIN)
	cl_assert_equal_s("builtin", sha1);
#else
	cl_assert(0);
#endif

#if defined(GIT_EXPERIMENTAL_SHA256) && defined(GIT_SHA256_BUILTIN)
	cl_assert_equal_s("builtin", sha256);
#elif defined(GIT_EXPERIMENTAL_SHA256)
	cl_assert(0);
#else
	cl_assert(sha256 == NULL);
#endif
}
