/*
 * Copyright (C) 2014 Filipe David Borba Manana <fdmanana@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <crypto/hash.h>
#include <linux/err.h>
#include "hash.h"

static struct crypto_shash *tfm;

int __init btrfs_hash_init(void)
{
	tfm = crypto_alloc_shash("crc32c", 0, 0);
	if (IS_ERR(tfm))
		return PTR_ERR(tfm);

	return 0;
}

void btrfs_hash_exit(void)
{
	crypto_free_shash(tfm);
}

u32 btrfs_crc32c(u32 crc, const void *address, unsigned int length)
{
	char desc[sizeof(struct shash_desc)
		+ crypto_shash_descsize(tfm)
		+ CRYPTO_MINALIGN] CRYPTO_MINALIGN_ATTR;
	struct shash_desc *shash = (struct shash_desc *)desc;
	u32 *ctx = (u32 *)shash_desc_ctx(shash);
	int err;

	shash->tfm = tfm;
	shash->flags = 0;
	*ctx = crc;

	err = crypto_shash_update(shash, address, length);
	BUG_ON(err);

	return *ctx;
}
