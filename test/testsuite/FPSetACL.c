/* Test suite for FPSetACL */
#include <sys/types.h>
#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

/* ------------------------- */
STATIC void test539()
{
    uint16_t vol = VolID;
    const DSI *dsi;
    char *dir = "test539_dir";
    int ret;
    darwin_ace_t ace;
    unsigned char user_uuid[16];
    dsi = &Conn->dsi;
    ENTER_TEST

    /* Check AFP version */
    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    /* Skip if volume doesn't support ACLs */
    if (!(get_vol_attrib(vol) & VOLPBIT_ATTR_ACLS)) {
        test_skipped(T_ACL);
        goto test_exit;
    }

    /* This test requires localhost connection to query user UUID from system */
    if (strcmp(Server, "localhost") != 0 && strcmp(Server, "127.0.0.1") != 0) {
        if (!Quiet) {
            fprintf(stdout, "\tTest requires localhost for user UUID lookup\n");
        }

        test_skipped(T_LOCALHOST);
        goto test_exit;
    }

    /* Create test directory */
    if (!FPCreateDir(Conn, vol, DIRDID_ROOT, dir)) {
        test_nottested();
        goto test_exit;
    }

    /* Verify filesystem actually supports ACLs */
    char test_path[1024];
    char cmd[2048];
    snprintf(test_path, sizeof(test_path), "/mnt/afpshare/%s", dir);
    snprintf(cmd, sizeof(cmd), "setfacl -m u::rwx %s 2>/dev/null", test_path);

    if (system(cmd) != 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFilesystem doesn't support ACLs\n");
        }

        test_skipped(T_ACL);
        goto fin;
    }

    /* Get UID for the AFP test user (User variable from command line) */
    snprintf(cmd, sizeof(cmd), "id -u '%s' 2>/dev/null", User);
    FILE *fp = popen(cmd, "r");

    if (!fp) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED: Could not query UID for user '%s'\n", User);
        }

        test_nottested();
        goto fin;
    }

    uid_t user_uid = 0;

    if (fscanf(fp, "%u", &user_uid) != 1) {
        pclose(fp);

        if (!Quiet) {
            fprintf(stdout, "\tFAILED: Could not parse UID for user '%s'\n", User);
        }

        test_nottested();
        goto fin;
    }

    pclose(fp);
    /* Generate UUID using Netatalk's localuuid_from_id() format
     * (from libatalk/acl/uuid.c:54) - this matches what the server uses:
     * - First 12 bytes: Fixed prefix for local user UUIDs
     * - Last 4 bytes: UID in network byte order */
    static const unsigned char local_user_prefix[12] = {
        0xff, 0xff, 0xee, 0xee, 0xdd, 0xdd,
        0xcc, 0xcc, 0xbb, 0xbb, 0xaa, 0xaa
    };
    memcpy(user_uuid, local_user_prefix, 12);
    uint32_t uid_network = htonl(user_uid);
    memcpy(user_uuid + 12, &uid_network, 4);

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t  Using UUID for user '%s' (UID %u)\n", User, user_uid);
    }

    /* === Test 1: Set ACL === */
    /* Build simple ACL: one ACE with read/write permissions */
    memset(&ace, 0, sizeof(ace));
    memcpy(ace.darwin_ace_uuid, user_uuid, 16);
    ace.darwin_ace_flags = htonl(DARWIN_ACE_FLAGS_PERMIT);
    ace.darwin_ace_rights = htonl(DARWIN_ACE_READ_DATA | DARWIN_ACE_WRITE_DATA);

    if (!Quiet && Verbose) {
        fprintf(stdout,
                "\t  Setting ACL: flags=0x%08x rights=0x%08x (READ_DATA | WRITE_DATA)\n",
                ntohl(ace.darwin_ace_flags), ntohl(ace.darwin_ace_rights));
    }

    /* Set ACL on directory */
    ret = AFPSetACL(Conn, vol, DIRDID_ROOT,
                    kFileSec_ACL,  /* Set ACL */
                    dir,           /* Directory name */
                    1,             /* One ACE */
                    &ace);

    if (ret != AFP_OK) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED AFPSetACL (set) returned %d\n", ret);
        }

        test_failed();
        goto fin;
    }

    /* Verify ACL was written to filesystem using Linux getfacl */
    if (!Quiet && Verbose) {
        snprintf(cmd, sizeof(cmd), "getfacl %s 2>&1 | grep -E 'user::|group::|other::'",
                 test_path);
        int getfacl_test = system(cmd);

        if (getfacl_test != 0) {
            fprintf(stdout, "\t  WARNING: getfacl check failed\n");
        }
    }

    /* === Test 2: Get ACL and verify it matches what we set === */

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t  ACL set with READ_DATA | WRITE_DATA permissions\n");
    }

    /* Get ACL from directory - should return the ACL we just set */
    ret = FPGetACL(Conn, vol, DIRDID_ROOT, kFileSec_ACL, dir);

    if (ret != AFP_OK) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPGetACL after set returned %d\n", ret);
        }

        test_failed();
        goto fin;
    }

    /* Parse and validate returned ACL data */
    {
        uint16_t returned_bitmap;
        uint32_t returned_ace_count;
        uint32_t returned_acl_flags;
        darwin_ace_t returned_ace;
        int ofs = 0;
        /* Parse bitmap */
        memcpy(&returned_bitmap, dsi->data + ofs, sizeof(returned_bitmap));
        returned_bitmap = ntohs(returned_bitmap);
        ofs += sizeof(returned_bitmap);

        if (!(returned_bitmap & kFileSec_ACL)) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED ACL bitmap not set in response (0x%04x)\n",
                        returned_bitmap);
            }

            test_failed();
            goto fin;
        }

        /* Parse ACE count */
        memcpy(&returned_ace_count, dsi->data + ofs, sizeof(returned_ace_count));
        returned_ace_count = ntohl(returned_ace_count);
        ofs += sizeof(returned_ace_count);

        if (returned_ace_count != 1) {
            /* ALWAYS print diagnostic - this is a critical server bug */
            fprintf(stdout, "\tFAILED expected 1 ACE, got %u\n", returned_ace_count);
            fprintf(stdout,
                    "\t  SERVER BUG: Volume reports ACL support (VOLPBIT_ATTR_ACLS)\n");
            fprintf(stdout, "\t  Filesystem supports ACLs (Linux setfacl test passed)\n");
            fprintf(stdout,
                    "\t  AFPSetACL returns success, but FPGetACL returns no ACEs\n");
            fprintf(stdout,
                    "\t  Investigation needed in etc/afpd/acls.c - ACLs not persisted\n");
            test_failed();
            goto fin;
        }

        /* Parse ACL flags */
        memcpy(&returned_acl_flags, dsi->data + ofs, sizeof(returned_acl_flags));
        returned_acl_flags = ntohl(returned_acl_flags);
        ofs += sizeof(returned_acl_flags);
        /* Parse first ACE */
        memcpy(&returned_ace, dsi->data + ofs, sizeof(darwin_ace_t));

        /* Validate ACE flags (network byte order, already) */
        if (returned_ace.darwin_ace_flags != ace.darwin_ace_flags) {
            if (!Quiet) {
                fprintf(stdout,
                        "\tFAILED ACE flags mismatch (expected 0x%08x, got 0x%08x)\n",
                        ntohl(ace.darwin_ace_flags), ntohl(returned_ace.darwin_ace_flags));
            }

            test_failed();
            goto fin;
        }

        /* Validate ACE rights - POSIX ACLs expand coarse permissions to multiple Darwin rights
         * For example: ACL_READ  → READ_DATA | READ_ATTRIBUTES | READ_EXTATTRIBUTES | READ_SECURITY
         *              ACL_WRITE → WRITE_DATA | APPEND_DATA | WRITE_ATTRIBUTES | WRITE_EXTATTRIBUTES
         * So we verify that requested rights are PRESENT (subset), not exact match.
         */
        uint32_t returned_rights = ntohl(returned_ace.darwin_ace_rights);
        uint32_t expected_rights = ntohl(ace.darwin_ace_rights);

        if ((returned_rights & expected_rights) != expected_rights) {
            if (!Quiet) {
                fprintf(stdout,
                        "\tFAILED ACE rights missing requested permissions (expected 0x%08x to be subset of 0x%08x)\n",
                        expected_rights, returned_rights);
            }

            test_failed();
            goto fin;
        }

        if (!Quiet && Verbose) {
            fprintf(stdout, "\t  ✓ ACL validated: 1 ACE with correct flags and rights\n");
        }
    }
    /* === Test 3: Modify ACL (set different permissions) === */
    /* Build modified ACL: different permissions (write-only, no read) */
    memset(&ace, 0, sizeof(ace));
    memcpy(ace.darwin_ace_uuid, user_uuid, 16);  /* Same UUID, different rights */
    ace.darwin_ace_flags = htonl(DARWIN_ACE_FLAGS_PERMIT);
    ace.darwin_ace_rights = htonl(DARWIN_ACE_WRITE_DATA);  /* Write only, no read */

    if (!Quiet && Verbose) {
        fprintf(stdout,
                "\t  Modifying ACL: flags=0x%08x rights=0x%08x (WRITE_DATA only)\n",
                ntohl(ace.darwin_ace_flags), ntohl(ace.darwin_ace_rights));
    }

    /* Update ACL on directory */
    ret = AFPSetACL(Conn, vol, DIRDID_ROOT,
                    kFileSec_ACL,  /* Set ACL (replaces existing) */
                    dir,
                    1,
                    &ace);

    if (ret != AFP_OK) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED AFPSetACL (modify) returned %d\n", ret);
        }

        test_failed();
        goto fin;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t  ACL modified to WRITE_DATA only (no READ_DATA)\n");
    }

    /* Verify modified ACL can be retrieved and matches modified ACE */
    ret = FPGetACL(Conn, vol, DIRDID_ROOT, kFileSec_ACL, dir);

    if (ret != AFP_OK) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPGetACL after modify returned %d\n", ret);
        }

        test_failed();
        goto fin;
    }

    /* Parse and validate modified ACL */
    {
        uint16_t returned_bitmap;
        uint32_t returned_ace_count;
        uint32_t returned_acl_flags;
        darwin_ace_t returned_ace;
        int ofs = 0;
        /* Parse bitmap */
        memcpy(&returned_bitmap, dsi->data + ofs, sizeof(returned_bitmap));
        returned_bitmap = ntohs(returned_bitmap);
        ofs += sizeof(returned_bitmap);

        if (!(returned_bitmap & kFileSec_ACL)) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED ACL bitmap not set after modify\n");
            }

            test_failed();
            goto fin;
        }

        /* Parse ACE count */
        memcpy(&returned_ace_count, dsi->data + ofs, sizeof(returned_ace_count));
        returned_ace_count = ntohl(returned_ace_count);
        ofs += sizeof(returned_ace_count);

        if (returned_ace_count != 1) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED expected 1 ACE after modify, got %u\n",
                        returned_ace_count);
            }

            test_failed();
            goto fin;
        }

        /* Parse ACL flags */
        memcpy(&returned_acl_flags, dsi->data + ofs, sizeof(returned_acl_flags));
        ofs += sizeof(returned_acl_flags);
        /* Parse ACE */
        memcpy(&returned_ace, dsi->data + ofs, sizeof(darwin_ace_t));
        /* Validate modified ACE rights - verify WRITE_DATA present and READ_DATA absent.
         * POSIX ACLs will expand WRITE to include APPEND_DATA, WRITE_ATTRIBUTES, etc.
         * We verify:
         * 1. Requested right (WRITE_DATA) is present
         * 2. Removed right (READ_DATA) is absent - proves modification worked
         */
        uint32_t returned_rights_mod = ntohl(returned_ace.darwin_ace_rights);
        uint32_t expected_rights_mod = ntohl(ace.darwin_ace_rights);
        uint32_t read_data_rights = htonl(DARWIN_ACE_READ_DATA);

        if ((returned_rights_mod & expected_rights_mod) != expected_rights_mod) {
            if (!Quiet) {
                fprintf(stdout,
                        "\tFAILED ACE rights missing WRITE_DATA (expected 0x%08x present in 0x%08x)\n",
                        expected_rights_mod, returned_rights_mod);
            }

            test_failed();
            goto fin;
        }

        if (returned_rights_mod & ntohl(read_data_rights)) {
            if (!Quiet) {
                fprintf(stdout,
                        "\tFAILED ACE still has READ_DATA (0x%08x) - modification didn't remove it\n",
                        returned_rights_mod);
            }

            test_failed();
            goto fin;
        }

        if (!Quiet && Verbose) {
            fprintf(stdout,
                    "\t  ✓ Modified ACL validated: 1 ACE with WRITE_DATA only (proves modification worked)\n");
        }
    }
    /* === Test 4: Remove ACL === */
    /* Remove ACL from directory */
    ret = AFPSetACL(Conn, vol, DIRDID_ROOT,
                    kFileSec_REMOVEACL,  /* Remove ACL */
                    dir,
                    0,
                    NULL);

    if (ret != AFP_OK) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED AFPSetACL (remove) returned %d\n", ret);
        }

        test_failed();
        goto fin;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t  ACL removed\n");
    }

    /* === Test 5: Verify ACL was removed === */
    /* Get ACL after removal - should return empty ACL or appropriate indication */
    ret = FPGetACL(Conn, vol, DIRDID_ROOT, kFileSec_ACL, dir);

    if (ret == AFP_OK) {
        /* Parse response to verify ACL list is empty */
        uint16_t returned_bitmap;
        uint32_t returned_ace_count;
        int ofs = 0;
        /* Parse bitmap */
        memcpy(&returned_bitmap, dsi->data + ofs, sizeof(returned_bitmap));
        returned_bitmap = ntohs(returned_bitmap);
        ofs += sizeof(returned_bitmap);

        if (returned_bitmap & kFileSec_ACL) {
            /* ACL bitmap set, check ACE count */
            memcpy(&returned_ace_count, dsi->data + ofs, sizeof(returned_ace_count));
            returned_ace_count = ntohl(returned_ace_count);

            if (returned_ace_count != 0) {
                if (!Quiet) {
                    fprintf(stdout,
                            "\tFAILED ACL should be empty after removal, got %u ACEs\n",
                            returned_ace_count);
                }

                test_failed();
                goto fin;
            }

            if (!Quiet && Verbose) {
                fprintf(stdout, "\t  ✓ ACL removed: FPGetACL returned 0 ACEs\n");
            }
        } else {
            if (!Quiet && Verbose) {
                fprintf(stdout, "\t  ✓ ACL removed: ACL bitmap not set\n");
            }
        }
    } else {
        /* Some servers may return an error for missing ACL - also acceptable */
        if (!Quiet && Verbose) {
            fprintf(stdout,
                    "\t  ✓ ACL removed: FPGetACL returned error %d (acceptable)\n", ret);
        }
    }

fin:
    FPDelete(Conn, vol, DIRDID_ROOT, dir);
test_exit:
    exit_test("FPSetACL:test539: ACL lifecycle (set, get, modify, remove)");
}

/* ----------- */
void FPSetACL_test()
{
    ENTER_TESTSET
    test539();
}
