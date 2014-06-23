#include "hll_file_consumer.h"

const int MAXIMUM_CSV_LINE_LENGTH = 5000;

size_t compute_hash_length(View view, char** fields) {
    size_t length = 0;
    uint index;
    for (uint i = 0; i < view.fields_count; i++) {
        index = view.fields_indices[i];
        length += strlen(fields[index]);
        length += (index + 1) > 9 ? 2 : 1;
        length++;
    }
    length += view.fields_count - 1;
    return length;
}

char *build_hash_id(View view, char** fields) {
    size_t length = compute_hash_length(view, fields) + 1;
    char *newstring = (char*) calloc(length, sizeof(char));
    size_t j = 0;
    uint index;
    uint csvindex;
    size_t field_length;
    for (uint i = 0; i < view.fields_count; i++) {
        index = view.fields_indices[i];
        csvindex = index + 1;
        if (csvindex > 9) {
            newstring[j++] = '0' + (csvindex / 10);
        }
        newstring[j++] = '0' + (csvindex % 10);
        newstring[j++] = '_';
        field_length = strlen(fields[index]);
        memcpy(&newstring[j], fields[index], field_length * sizeof(char));
        j += field_length;
        newstring[j++] = ',';
    }
    newstring[j - 1] = '\0';
    return newstring;
}

void process_file(const char *path, HllDictionary ***hlls_table, uint b) {
    SimpleCSVParser parser;
    Dstats stats;
    HllDictionary *hll_for_the_id;
    Hyperloglog *hll = NULL;
    uint64_t digest_value;
    char *hash_id;
    
    int hour = get_hour_from_dstats(path);
    
    init_parser(&parser, try_fopen(path), MAXIMUM_CSV_LINE_LENGTH, 29, '\t');
    while (next_line(&parser)) {
        parse_line(parser.fields, &stats);
        for (uint i = 0; i < VIEWS_COUNT; i++) {
            hash_id = build_hash_id(views[i], parser.fields);
            hll_for_the_id = find_hll(hash_id, hlls_table[hour]);
            
            if (hll_for_the_id == NULL) {
                hll = create_hll(b);
                add_hll_to_dict(hash_id, hll, hlls_table[hour]);
            } else {
                hll = hll_for_the_id->hll;
                free(hash_id);
            }
            
            digest_value = MurmurHash64A(stats.uuid, (int)strlen(stats.uuid), 42);
            updateM(hll, digest_value);
        }
        
    }
    
    free_parser(&parser);
}

void print_results(HllDictionary *hlls_table, uint b) {
    HllDictionary *h, *tmp;
    uint card;
    uint64_t bytes_sum = 0;
    uint i = 0;
    byte *compressed = (byte*) malloc(1 << b);
    HASH_ITER(hh, hlls_table, h, tmp) {
        i++;
        card = estimate_cardinality(h->hll);
        // printf("%s:%u\n", h->hash_id, card);
        // save_sparse(h->hll, h->hash_id);
        bytes_sum += compress_hll(h->hll, compressed);
    }
    printf("Celkovy pocet megabytu:             %g\n", bytes_sum / (1024*1024.0));
    printf("Prumerna velikost vektoru v bytech: %g\n", (bytes_sum / (double)i));
}

void process_all_files(tinydir_dir dir, HllDictionary ***hlls_table, uint b) {
    uint counter = 0;
    
    while (dir.has_next) {
        counter++;
        if (counter % 10 == 0)
        {
            printf("Zpracoval jsem %u souboru.\n", counter);
        }
        tinydir_file file;
        if (tinydir_readfile(&dir, &file) == -1) {
            perror("Error getting file");
            return;
        }
        
        if (file.name[0] != '.') {
            process_file(file.path, hlls_table, b);
        }
        
		tinydir_next(&dir);
	}
}

void hyperloglog(uint b, const char *path) {
    HllDictionary *tables[24];
    HllDictionary ***ptr_tables = (HllDictionary ***) malloc(sizeof(HllDictionary **) * 24);
    for (int i = 0; i < 24; i++) {
        tables[i] = create_empty_hll_dict();
        ptr_tables[i] = &tables[i];
    }
    
    tinydir_dir dir;
    if (try_open_dir(&dir, path)) {
        process_all_files(dir, ptr_tables, b);
        for (int i = 0; i < 24; i++) {
            printf("%i. hodina\n", i);
            print_results(tables[i], b);
        }
    }
}