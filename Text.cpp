tpb_read			tpb_write
tpb_nowait			tpb_wait
tpb_autocommit
tpb_no_auto_undo

//  fill out a standard tpb buffer ahead of time so we know
//  how large it is 

	text = tpb_buffer;
	*text++ = gds_tpb_version1;
	*text++ = (trans->tra_flags & TRA_ro) ? gds_tpb_read : gds_tpb_write;
	if (trans->tra_flags & TRA_con)
		*text++ = gds_tpb_consistency;
	else if (trans->tra_flags & TRA_read_committed)
		*text++ = gds_tpb_read_committed;
	else
		*text++ = gds_tpb_concurrency;
	*text++ = (trans->tra_flags & TRA_nw) ? gds_tpb_nowait : gds_tpb_wait;

	if (trans->tra_flags & TRA_read_committed)
		*text++ =
			(trans->
			 tra_flags & TRA_rec_version) ? gds_tpb_rec_version :
			gds_tpb_no_rec_version;

	if (trans->tra_flags & TRA_autocommit)
		*text++ = gds_tpb_autocommit;
	if (trans->tra_flags & TRA_no_auto_undo)
		*text++ = gds_tpb_no_auto_undo;
	*text = 0;
	tpb_len = text - tpb_buffer;
