static void header(unsigned char *sha1, char *path1, int mode1,
		   unsigned char *sha2, char *path2, int mode2)
{
	char *abbrev1, *abbrev2;
	int subproject;

	subproject = (S_ISDIRLNK(mode1) || S_ISDIRLNK(mode2));
	html("<tr><td>");
	html("<div class='head'>");
	html("diff --git a/");
	html_txt(path1);
	html(" b/");
	html_txt(path2);

	if (is_null_sha1(sha1))
		path1 = "dev/null";
	if (is_null_sha1(sha2))
		path2 = "dev/null";

	if (mode1 == 0)
		htmlf("<br/>new file mode %.6o", mode2);

	if (mode2 == 0)
		htmlf("<br/>deleted file mode %.6o", mode1);

	if (!subproject) {
		abbrev1 = xstrdup(find_unique_abbrev(sha1, DEFAULT_ABBREV));
		abbrev2 = xstrdup(find_unique_abbrev(sha2, DEFAULT_ABBREV));
		htmlf("<br/>index %s..%s", abbrev1, abbrev2);
		free(abbrev1);
		free(abbrev2);
		if (mode1 != 0 && mode2 != 0) {
			htmlf(" %.6o", mode1);
			if (mode2 != mode1)
				htmlf("..%.6o", mode2);
		}
		html("<br/>--- a/");
		html_txt(path1);
		html("<br/>+++ b/");
		html_txt(path2);
	}
	html("</div>");
}

static void filepair_cb(struct diff_filepair *pair)
{
	header(pair->one->sha1, pair->one->path, pair->one->mode,
	       pair->two->sha1, pair->two->path, pair->two->mode);
	if (S_ISDIRLNK(pair->one->mode) || S_ISDIRLNK(pair->two->mode)) {
		if (S_ISDIRLNK(pair->one->mode))
			print_line(fmt("-Subproject %s", sha1_to_hex(pair->one->sha1)), 52);
		if (S_ISDIRLNK(pair->two->mode))
			print_line(fmt("+Subproject %s", sha1_to_hex(pair->two->sha1)), 52);
		return;
	}
	if (cgit_diff_files(pair->one->sha1, pair->two->sha1, print_line))
		cgit_print_error("Error running diff");
}

void cgit_print_diff(const char *head, const char *old_hex, const char *new_hex, char *path)
	enum object_type type;
	unsigned long size;
	struct commit *commit;

	if (head && !old_hex && !new_hex) {
		get_sha1(head, sha1);
		commit = lookup_commit_reference(sha1);
		if (commit && !parse_commit(commit)) {
			html("<table class='diff'>");
			cgit_diff_commit(commit, filepair_cb);
			html("</td></tr></table>");
		}
		return;
	}
	type = sha1_object_info(sha1, &size);
	if (type == OBJ_BAD) {
		type = sha1_object_info(sha2, &size);
		if (type == OBJ_BAD) {
			cgit_print_error(fmt("Bad object names: %s, %s", old_hex, new_hex));
			return;
		}
	}

	html("<table class='diff'>");
	switch(type) {
	case OBJ_BLOB:
		html("<tr><td>");
		header(sha1, path, 0644, sha2, path, 0644);
		if (cgit_diff_files(sha1, sha2, print_line))
			cgit_print_error("Error running diff");
		html("</tr></td>");
		break;
	case OBJ_TREE:
		cgit_diff_tree(sha1, sha2, filepair_cb);
		break;
	default:
		cgit_print_error(fmt("Unhandled object type: %s",
				     typename(type)));
		break;
	}