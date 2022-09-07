
uint32_t nmbm_debug_get_block_state(struct nmbm_instance *ni, uint32_t ba)
{
	return nmbm_get_block_state(ni, ba);
}

char nmbm_debug_get_phys_block_type(struct nmbm_instance *ni, uint32_t ba)
{
	uint32_t eba, limit;
	bool success;

	if (nmbm_get_block_state(ni, ba) == BLOCK_ST_BAD)
		return NMBM_BLOCK_BAD;

	if (ba < ni->data_block_count)
		return NMBM_BLOCK_GOOD_DATA;

	if (ba == ni->signature_ba)
		return NMBM_BLOCK_SIGNATURE;

	if (ni->main_table_ba) {
		limit = ni->backup_table_ba ? ni->backup_table_ba :
			ni->mapping_blocks_ba;

		success = nmbm_block_walk_asc(ni, ni->main_table_ba, &eba,
			size2blk(ni, ni->info_table_size), limit);

		if (success && ba >= ni->main_table_ba && ba < eba)
			return NMBM_BLOCK_MAIN_INFO_TABLE;
	}

	if (ba >= ni->backup_table_ba && ba < ni->mapping_blocks_ba)
		return NMBM_BLOCK_BACKUP_INFO_TABLE;

	if (ba > ni->mapping_blocks_top_ba && ba < ni->signature_ba)
		return NMBM_BLOCK_REMAPPED;

	return NMBM_BLOCK_GOOD_MGMT;
}
