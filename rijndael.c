#define MODULE_NAME "encryption"
#define MAKING_ENCRYPTION

#include "src/mod/module.h"
#include "rijndael.h"

#include "aes.h"
#undef BLOCK_SIZE
#define BLOCK_SIZE 128

#undef global
static Function *global = NULL;

static aes aes_cx;
static int key_size = 16;

// Yeah right. Maybe later!
static int rijndael_expmem()
{
	return(0);
}

static void rijndael_report(int idx, int details)
{
  if (details) {
    dprintf(idx, "    Rijndael encryption module:\n"
                 "    Thanks for using Rijndael! You rock!\n");
  }
}

/* Convert 64-bit encrypted password to text for userfile */
static char *base64 = "./0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

static int base64dec(char c)
{
  int i;

  for (i = 0; i < 64; i++)
    if (base64[i] == c)
      return i;
  return 0;
}

// Initializes aes_cx.
static void rijndael_init (char *key, int keylen)
{
	char maxkey[32];

	memset(maxkey, 0, 32);
	memcpy(maxkey, key, (keylen > 32) ? 32 : keylen);
	memset(&aes_cx, 0, sizeof(aes_cx));
	set_key((const byte *)maxkey, key_size, 3, &aes_cx);
}

static void rijndael_encipher (char *input, int inputlen)
{
	char output[BLOCK_SIZE/8];

	ncrypt((const byte *)input, (byte *)output, &aes_cx);
	memcpy(input, output, BLOCK_SIZE/8);
}

static void rijndael_decipher (char *input, int inputlen)
{
	char output[BLOCK_SIZE/8];

	decrypt((const byte *)input, (byte *)output, &aes_cx);
	memcpy(input, output, BLOCK_SIZE/8);
}

static void rijndael_encrypt_pass(char *text, char *new)
{
	int i, left, right;
	char *p, block[BLOCK_SIZE/8];

	i = strlen(text);
	if (i < BLOCK_SIZE/8) memset(block, 0, BLOCK_SIZE/8);
	else i = BLOCK_SIZE/8;
	memcpy(block, text, i);

	rijndael_init(text, strlen(text));
	rijndael_encipher(block, BLOCK_SIZE/8);

	p = new;
	*p++ = '+';			/* + means encrypted pass */
	memcpy(&right, block, sizeof(right));
	memcpy(&left, block+sizeof(right), sizeof(left));
	for (i = 0; i < 6; i++) {
		*p++ = base64[right & (int) 0x3f];
		right = (right >> 6);
		*p++ = base64[left & (int) 0x3f];
		left = (left >> 6);
	}
	*p = 0;
}

/* Returned string must be freed when done with it!
 */
static char *encrypt_string(char *key, char *str)
{
	char *input_, *input, *output, *output_, block[BLOCK_SIZE/8];
	int i, j, k;

	k = strlen(str);
	input_ = nmalloc(k+BLOCK_SIZE/8+1);
	memcpy(input_, str, k);
	memset(input_+k, 0, BLOCK_SIZE/8+1);
	if ((!key) || (!key[0])) return(input_);

	output_ = nmalloc((k + BLOCK_SIZE/8 + 1) * 2);

	input = input_;
	output = output_;

	rijndael_init(key, strlen(key));

	while (*input) {
		memcpy(block, input, BLOCK_SIZE/8);
		input += BLOCK_SIZE/8;
		rijndael_encipher(block, BLOCK_SIZE/8);
		for (i = 0; i < BLOCK_SIZE/8; i += 4) {
			memcpy(&k, block+i, 4);
			for (j = 0; j < 6; j++) {
				*output++ = base64[k & 0x3f];
				k = (k >> 6);
			}
		}
	}
	*output = 0;
	nfree(input_);
	return(output_);
}

/* Returned string must be freed when done with it!
 */
static char *decrypt_string(char *key, char *str)
{
	char *input, *input_, *output, *output_, block[BLOCK_SIZE/8];
	int i, j, k;

	i = strlen(str);
	input_ = nmalloc(i + BLOCK_SIZE/8+21);
	memcpy(input_, str, i);
	memset(input_+i, 0, BLOCK_SIZE/8+21);
	if ((!key) || (!key[0])) return(input_);

	output_ = nmalloc(i + BLOCK_SIZE/8+21);
	memset(output_, 0, i+BLOCK_SIZE/8+21);

	rijndael_init(key, strlen(key));

	input = input_;
	output = output_;

	while (*input) {
		for (i = 0; i < BLOCK_SIZE/8; i += 4) {
			k = 0;
			for (j = 0; j < 6; j++)
				k |= (base64dec(*input++)) << (j * 6);
			memcpy(block+i, &k, 4);
		}
		rijndael_decipher(block, BLOCK_SIZE/8);
		for (i = 0; i < BLOCK_SIZE/8; i += 4) {
			memcpy(&k, block+i, 4);
			for (j = 3; j >= 0; j--)
				*output++ = (k & (0xff << ((3 - j) * 8))) >> ((3 - j) * 8);
		}
	}
	*output = 0;
	nfree(input_);
	return(output_);
}

static int tcl_encrypt STDVAR
{
  char *p;

  BADARGS(3, 3, " key string");
  p = encrypt_string(argv[1], argv[2]);
  Tcl_AppendResult(irp, p, NULL);
  nfree(p);
  return TCL_OK;
}

static int tcl_decrypt STDVAR
{
  char *p;

  BADARGS(3, 3, " key string");
  p = decrypt_string(argv[1], argv[2]);
  Tcl_AppendResult(irp, p, NULL);
  nfree(p);
  return TCL_OK;
}

static int tcl_encpass STDVAR
{
  BADARGS(2, 2, " string");
  if (strlen(argv[1]) > 0) {
    char p[16];
    rijndael_encrypt_pass(argv[1], p);
    Tcl_AppendResult(irp, p, NULL);
  } else
    Tcl_AppendResult(irp, "", NULL);
  return TCL_OK;
}

static int tcl_set_key_size STDVAR
{
	int newsize;
	BADARGS(2, 2, " key_size (128, 192, 256)");
	newsize = atoi(argv[1]);
	if (newsize != 128 && newsize != 192 && newsize != 256) {
		Tcl_AppendResult(irp, "error: key size must be 128, 192, or 256", NULL);
		return TCL_ERROR;
	}
	Tcl_AppendResult(irp, "", NULL);
	key_size = newsize/8;
	return TCL_OK;
}

static tcl_cmds mytcls[] =
{
  {"encrypt",	tcl_encrypt},
  {"decrypt",	tcl_decrypt},
  {"encpass",	tcl_encpass},
  {"set_key_size", tcl_set_key_size},
  {NULL,	NULL}
};

/* You CANT -module an encryption module , so -module just resets it.
 */
static char *rijndael_close()
{
  return "You can't unload an encryption module";
}

char *rijndael_start(Function *);

static Function rijndael_table[] =
{
  /* 0 - 3 */
  (Function) rijndael_start,
  (Function) rijndael_close,
  (Function) rijndael_expmem,
  (Function) rijndael_report,
  /* 4 - 7 */
  (Function) encrypt_string,
  (Function) decrypt_string,
};

char *rijndael_start(Function *global_funcs)
{
  /* `global_funcs' is NULL if eggdrop is recovering from a restart.
   *
   * As the encryption module is never unloaded, only initialise stuff
   * that got reset during restart, e.g. the tcl bindings.
   */
  if (global_funcs) {
    global = global_funcs;

    if (!module_rename("rijndael", MODULE_NAME))
      return "Already loaded.";

    module_register(MODULE_NAME, rijndael_table, 2, 3);
    if (!module_depend(MODULE_NAME, "eggdrop", 108, 0)) {
      module_undepend(MODULE_NAME);
      return "This module requires Eggdrop 1.8.0 or later.";
    }
    add_hook(HOOK_ENCRYPT_PASS, (Function) rijndael_encrypt_pass);
    add_hook(HOOK_ENCRYPT_STRING, (Function) encrypt_string);
    add_hook(HOOK_DECRYPT_STRING, (Function) decrypt_string);
  }
  add_tcl_commands(mytcls);
  return NULL;
}
