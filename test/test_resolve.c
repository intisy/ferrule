#include "greatest.h"
#include "manifest.h"
#include "registry.h"
#include "resolve.h"
#include "source_path.h"

#include <string.h>

static fr_registry *with_path_source(void) {
    fr_registry *registry = fr_registry_create();
    fr_error err;
    fr_registry_add_source(registry, &FR_SOURCE_PATH, &err);
    return registry;
}

TEST pulls_in_transitive_requires(void) {
    fr_manifest manifest; fr_error err;
    ASSERT_EQ(FR_OK, fr_manifest_read("test/fixtures/consumer/ferrule.json", &manifest, &err));
    fr_registry *registry = with_path_source();

    fr_resolved *items = NULL; size_t count = 0;
    ASSERT_EQ(FR_OK, fr_resolve_consumer(&manifest, &manifest.consumers[0],
                                         "test/fixtures/consumer", registry, &items, &count, &err));
    ASSERT_EQ(2, (int) count);
    ASSERT_STR_EQ("contracts", items[0].module);
    ASSERT_STR_EQ("ir", items[1].module);
    ASSERT_STR_EQ("intisy-ai:basekit:5.0.0:contracts", items[0].gradle_coordinate);

    fr_resolved_free(items, count);
    fr_registry_destroy(registry);
    fr_manifest_free(&manifest);
    PASS();
}

TEST deduplicates_a_module_reached_twice(void) {
    fr_manifest manifest; fr_error err;
    fr_manifest_read("test/fixtures/consumer-dup/ferrule.json", &manifest, &err);
    fr_registry *registry = with_path_source();

    fr_resolved *items = NULL; size_t count = 0;
    ASSERT_EQ(FR_OK, fr_resolve_consumer(&manifest, &manifest.consumers[0],
                                         "test/fixtures/consumer-dup", registry, &items, &count, &err));
    ASSERT_EQ(2, (int) count);

    fr_resolved_free(items, count);
    fr_registry_destroy(registry);
    fr_manifest_free(&manifest);
    PASS();
}

TEST rejects_a_version_outside_the_range(void) {
    fr_manifest manifest; fr_error err;
    fr_manifest_read("test/fixtures/consumer-badrange/ferrule.json", &manifest, &err);
    fr_registry *registry = with_path_source();

    fr_resolved *items = NULL; size_t count = 0;
    ASSERT_EQ(FR_ERR, fr_resolve_consumer(&manifest, &manifest.consumers[0],
                                          "test/fixtures/consumer-badrange", registry, &items, &count, &err));
    ASSERT(strstr(err.message, "intisy-ai/basekit") != NULL);
    ASSERT(strstr(err.message, "5.0.0") != NULL);

    fr_registry_destroy(registry);
    fr_manifest_free(&manifest);
    PASS();
}

TEST reports_a_module_absent_from_the_language(void) {
    fr_manifest manifest; fr_error err;
    fr_manifest_read("test/fixtures/consumer-noloader/ferrule.json", &manifest, &err);
    fr_registry *registry = with_path_source();

    fr_resolved *items = NULL; size_t count = 0;
    ASSERT_EQ(FR_ERR, fr_resolve_consumer(&manifest, &manifest.consumers[0],
                                          "test/fixtures/consumer-noloader", registry, &items, &count, &err));
    ASSERT(strstr(err.message, "loader") != NULL);
    ASSERT(strstr(err.message, "gradle") != NULL);

    fr_registry_destroy(registry);
    fr_manifest_free(&manifest);
    PASS();
}

TEST reports_an_unknown_module_by_name(void) {
    fr_manifest manifest; fr_error err;
    fr_manifest_read("test/fixtures/consumer-unknown/ferrule.json", &manifest, &err);
    fr_registry *registry = with_path_source();

    fr_resolved *items = NULL; size_t count = 0;
    ASSERT_EQ(FR_ERR, fr_resolve_consumer(&manifest, &manifest.consumers[0],
                                          "test/fixtures/consumer-unknown", registry, &items, &count, &err));
    ASSERT(strstr(err.message, "nope") != NULL);

    fr_registry_destroy(registry);
    fr_manifest_free(&manifest);
    PASS();
}

TEST rejects_a_project_that_does_not_match_its_source(void) {
    fr_manifest manifest; fr_error err;
    fr_manifest_read("test/fixtures/consumer-mismatch/ferrule.json", &manifest, &err);
    fr_registry *registry = with_path_source();

    fr_resolved *items = NULL; size_t count = 0;
    ASSERT_EQ(FR_ERR, fr_resolve_consumer(&manifest, &manifest.consumers[0],
                                          "test/fixtures/consumer-mismatch", registry, &items, &count, &err));
    ASSERT(strstr(err.message, "intisy-ai/basekit") != NULL);
    ASSERT(strstr(err.message, "intisy-ai/not-basekit") != NULL);

    fr_registry_destroy(registry);
    fr_manifest_free(&manifest);
    PASS();
}

TEST rejects_an_unsupported_language_even_with_no_modules(void) {
    fr_manifest manifest; fr_error err;
    fr_manifest_read("test/fixtures/consumer-badlanguage/ferrule.json", &manifest, &err);
    fr_registry *registry = with_path_source();

    fr_resolved *items = NULL; size_t count = 0;
    ASSERT_EQ(FR_ERR, fr_resolve_consumer(&manifest, &manifest.consumers[0],
                                          "test/fixtures/consumer-badlanguage", registry, &items, &count, &err));
    ASSERT(strstr(err.message, "npm") != NULL);

    fr_registry_destroy(registry);
    fr_manifest_free(&manifest);
    PASS();
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_TEST(pulls_in_transitive_requires);
    RUN_TEST(deduplicates_a_module_reached_twice);
    RUN_TEST(rejects_a_version_outside_the_range);
    RUN_TEST(reports_a_module_absent_from_the_language);
    RUN_TEST(reports_an_unknown_module_by_name);
    RUN_TEST(rejects_a_project_that_does_not_match_its_source);
    RUN_TEST(rejects_an_unsupported_language_even_with_no_modules);
    GREATEST_MAIN_END();
}
