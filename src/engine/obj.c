#include "engine/obj.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_CAPACITY 16384

// one face corner as 0-based indices into the v, vt and vn lists, -1 where the face gives none
typedef struct {
    int v;
    int vt;
    int vn;
} Corner;

typedef struct {
    float u;
    float v;
} Uv;

typedef struct {
    Corner corner;
    int index;
} Slot;

typedef struct {
    Corner corner;
    Vec3 tangent;
    Vec3 bitangent;
} Pending;

typedef struct {
    int positions;
    int uvs;
    int normals;
    int corners;
    int face_corners;
    int indices;
    int groups;
} Counts;

typedef struct {
    Model *model;
    const char *path;
    int line;
    Vec3 *positions;
    Vec3 *position_normals;
    Uv *uvs;
    Vec3 *normals;
    int position_count;
    int uv_count;
    int normal_count;
    Corner *corners;
    Pending *pending;
    Slot *slots;
    unsigned int slot_mask;
    int material;
} Parser;

static int fail(const char *path, int line, const char *what)
{
    fprintf(stderr, "%s:%d: %s\n", path, line, what);
    return -1;
}

static char *trim(char *text)
{
    char *end;

    while (isspace((unsigned char)*text)) {
        text++;
    }
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) {
        end--;
    }
    *end = '\0';

    return text;
}

static char *split_key(char *line)
{
    char *comment = strchr(line, '#');
    char *rest = line;

    if (comment != NULL) {
        *comment = '\0';
    }
    while (*rest != '\0' && !isspace((unsigned char)*rest)) {
        rest++;
    }
    if (*rest != '\0') {
        *rest++ = '\0';
    }

    return trim(rest);
}

// fgets stops short of the newline on a line this long; the remainder would come back as a line of its own
static int line_overflowed(const char *line, size_t capacity)
{
    const size_t length = strlen(line);

    return length == capacity - 1 && line[length - 1] != '\n';
}

static int parse_floats(const char *text, float *out, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        char *end;

        out[i] = strtof(text, &end);
        if (end == text) {
            return -1;
        }
        text = end;
    }

    return 0;
}

static int count_tokens(const char *text)
{
    int count = 0;

    for (;;) {
        while (isspace((unsigned char)*text)) {
            text++;
        }
        if (*text == '\0') {
            return count;
        }
        count++;
        while (*text != '\0' && !isspace((unsigned char)*text)) {
            text++;
        }
    }
}

static void count_lines(FILE *file, Counts *counts)
{
    char line[LINE_CAPACITY];

    memset(counts, 0, sizeof *counts);
    while (fgets(line, sizeof line, file) != NULL) {
        const char *key = line;
        const char *rest = split_key(line);

        if (strcmp(key, "v") == 0) {
            counts->positions++;
        } else if (strcmp(key, "vt") == 0) {
            counts->uvs++;
        } else if (strcmp(key, "vn") == 0) {
            counts->normals++;
        } else if (strcmp(key, "f") == 0) {
            const int corners = count_tokens(rest);

            counts->corners += corners;
            if (corners > counts->face_corners) {
                counts->face_corners = corners;
            }
            if (corners >= 3) {
                counts->indices += 3 * (corners - 2);
            }
        } else if (strcmp(key, "g") == 0 || strcmp(key, "usemtl") == 0) {
            counts->groups++;
        }
    }
    rewind(file);
}

static void *alloc(int count, size_t size)
{
    // the spare element keeps an empty section of the file from asking calloc for zero bytes
    return calloc((size_t)count + 1, size);
}

static int allocate(Parser *p, const Counts *counts)
{
    MeshData *data = &p->model->data;
    unsigned int slot_count = 16;

    while (slot_count < 2u * (unsigned int)counts->corners) {
        slot_count *= 2;
    }
    p->slot_mask = slot_count - 1;

    p->positions = alloc(counts->positions, sizeof *p->positions);
    p->position_normals = alloc(counts->positions, sizeof *p->position_normals);
    p->uvs = alloc(counts->uvs, sizeof *p->uvs);
    p->normals = alloc(counts->normals, sizeof *p->normals);
    p->corners = alloc(counts->face_corners, sizeof *p->corners);
    p->pending = alloc(counts->corners, sizeof *p->pending);
    p->slots = malloc(sizeof *p->slots * slot_count);
    data->vertices = alloc(counts->corners, sizeof *data->vertices);
    data->indices = alloc(counts->indices, sizeof *data->indices);
    data->groups = alloc(counts->groups + 1, sizeof *data->groups);
    if (p->positions == NULL || p->position_normals == NULL || p->uvs == NULL || p->normals == NULL ||
        p->corners == NULL || p->pending == NULL || p->slots == NULL || data->vertices == NULL ||
        data->indices == NULL || data->groups == NULL) {
        return -1;
    }

    // every byte 0xff reads as index -1, the empty mark
    memset(p->slots, 0xff, sizeof *p->slots * slot_count);
    data->group_count = 1;
    data->groups[0].material = -1;

    return 0;
}

static void free_parser(Parser *p)
{
    free(p->positions);
    free(p->position_normals);
    free(p->uvs);
    free(p->normals);
    free(p->corners);
    free(p->pending);
    free(p->slots);
}

static int add_position(Parser *p, const char *text)
{
    float values[3];

    if (parse_floats(text, values, 3) != 0) {
        return fail(p->path, p->line, "bad vertex position");
    }
    p->positions[p->position_count++] = v3(values[0], values[1], values[2]);

    return 0;
}

static int add_uv(Parser *p, const char *text)
{
    float values[2];

    if (parse_floats(text, values, 2) != 0) {
        return fail(p->path, p->line, "bad texture coordinate");
    }
    p->uvs[p->uv_count].u = values[0];
    p->uvs[p->uv_count].v = values[1];
    p->uv_count++;

    return 0;
}

static int add_normal(Parser *p, const char *text)
{
    float values[3];

    if (parse_floats(text, values, 3) != 0) {
        return fail(p->path, p->line, "bad vertex normal");
    }
    p->normals[p->normal_count++] = v3(values[0], values[1], values[2]);

    return 0;
}

// v, v/vt, v//vn or v/vt/vn, 1-based, negative ones counting back from the last one defined
static int parse_corner(const Parser *p, const char **cursor, Corner *corner)
{
    const int counts[3] = {p->position_count, p->uv_count, p->normal_count};
    const char *text = *cursor;
    int fields[3] = {-1, -1, -1};
    int i;

    for (i = 0; i < 3; i++) {
        char *end;
        long index = strtol(text, &end, 10);

        if (end != text) {
            if (index < 0) {
                index += counts[i] + 1;
            }
            if (index < 1 || index > counts[i]) {
                return -1;
            }
            fields[i] = (int)index - 1;
        } else if (i == 0) {
            return -1;
        }
        text = end;
        if (*text != '/') {
            break;
        }
        text++;
    }
    if (*text != '\0' && !isspace((unsigned char)*text)) {
        return -1;
    }

    corner->v = fields[0];
    corner->vt = fields[1];
    corner->vn = fields[2];
    *cursor = text;

    return 0;
}

static unsigned int hash(Corner corner)
{
    const unsigned int mixed = (unsigned int)corner.v * 73856093u ^ (unsigned int)corner.vt * 19349663u ^
                               (unsigned int)corner.vn * 83492791u;

    return mixed ^ (mixed >> 16);
}

static unsigned int vertex_index(Parser *p, Corner corner)
{
    MeshData *data = &p->model->data;
    unsigned int slot = hash(corner) & p->slot_mask;
    MeshVertex *vertex;
    int index;

    while (p->slots[slot].index >= 0) {
        const Corner *key = &p->slots[slot].corner;

        if (key->v == corner.v && key->vt == corner.vt && key->vn == corner.vn) {
            return (unsigned int)p->slots[slot].index;
        }
        slot = (slot + 1) & p->slot_mask;
    }

    index = data->vertex_count++;
    vertex = &data->vertices[index];
    vertex->position = p->positions[corner.v];
    vertex->u = corner.vt >= 0 ? p->uvs[corner.vt].u : 0.0f;
    vertex->v = corner.vt >= 0 ? p->uvs[corner.vt].v : 0.0f;
    // files carry rounded normals; they are made unit on the way in
    vertex->normal = corner.vn >= 0 ? v3_normalize(p->normals[corner.vn]) : v3(0.0f, 0.0f, 0.0f);
    p->pending[index].corner = corner;
    p->slots[slot].corner = corner;
    p->slots[slot].index = index;

    return (unsigned int)index;
}

static void accumulate_tangent(Parser *p, const unsigned int *index, const Corner *corners, Vec3 edge1, Vec3 edge2)
{
    const Uv uv0 = p->uvs[corners[0].vt];
    const Uv uv1 = p->uvs[corners[1].vt];
    const Uv uv2 = p->uvs[corners[2].vt];
    const float du1 = uv1.u - uv0.u;
    const float dv1 = uv1.v - uv0.v;
    const float du2 = uv2.u - uv0.u;
    const float dv2 = uv2.v - uv0.v;
    const float area = du1 * dv2 - du2 * dv1;
    Vec3 tangent;
    Vec3 bitangent;
    int k;

    if (fabsf(area) < 1e-12f) {
        return;
    }
    tangent = v3_scale(v3_sub(v3_scale(edge1, dv2), v3_scale(edge2, dv1)), 1.0f / area);
    bitangent = v3_scale(v3_sub(v3_scale(edge2, du1), v3_scale(edge1, du2)), 1.0f / area);
    for (k = 0; k < 3; k++) {
        Pending *pending = &p->pending[index[k]];

        pending->tangent = v3_add(pending->tangent, tangent);
        pending->bitangent = v3_add(pending->bitangent, bitangent);
    }
}

static void add_triangle(Parser *p, const Corner *corners)
{
    MeshData *data = &p->model->data;
    unsigned int index[3];
    Vec3 edge1;
    Vec3 edge2;
    Vec3 normal;
    int k;

    for (k = 0; k < 3; k++) {
        index[k] = vertex_index(p, corners[k]);
        data->indices[data->index_count++] = index[k];
    }
    data->groups[data->group_count - 1].index_count += 3;

    edge1 = v3_sub(p->positions[corners[1].v], p->positions[corners[0].v]);
    edge2 = v3_sub(p->positions[corners[2].v], p->positions[corners[0].v]);
    // unnormalized, the cross product weights each face by its area in a shared normal
    normal = v3_cross(edge1, edge2);
    for (k = 0; k < 3; k++) {
        p->position_normals[corners[k].v] = v3_add(p->position_normals[corners[k].v], normal);
    }
    if (corners[0].vt >= 0 && corners[1].vt >= 0 && corners[2].vt >= 0) {
        accumulate_tangent(p, index, corners, edge1, edge2);
    }
}

static int add_face(Parser *p, const char *text)
{
    Corner triangle[3];
    int count = 0;
    int i;

    for (;;) {
        while (isspace((unsigned char)*text)) {
            text++;
        }
        if (*text == '\0') {
            break;
        }
        if (parse_corner(p, &text, &p->corners[count]) != 0) {
            return fail(p->path, p->line, "bad face vertex");
        }
        count++;
    }
    if (count < 3) {
        return fail(p->path, p->line, "face with fewer than 3 vertices");
    }

    triangle[0] = p->corners[0];
    for (i = 1; i + 1 < count; i++) {
        triangle[1] = p->corners[i];
        triangle[2] = p->corners[i + 1];
        add_triangle(p, triangle);
    }

    return 0;
}

static void begin_group(Parser *p, const char *name)
{
    MeshData *data = &p->model->data;
    MeshGroup *group = &data->groups[data->group_count - 1];

    // a group that has no faces yet is renamed instead, which is how empty groups vanish
    if (group->index_count > 0) {
        group = &data->groups[data->group_count++];
        group->first_index = data->index_count;
    }
    snprintf(group->name, sizeof group->name, "%s", name);
    group->material = p->material;
}

static int find_material(const Model *model, const char *name)
{
    int i;

    for (i = 0; i < model->material_count; i++) {
        if (strcmp(model->materials[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}

static void use_material(Parser *p, const char *name)
{
    MeshData *data = &p->model->data;
    MeshGroup *group = &data->groups[data->group_count - 1];

    p->material = find_material(p->model, name);
    // a material change inside a group starts a new group under the same name: one material per group
    if (group->index_count > 0 && group->material != p->material) {
        MeshGroup *split = &data->groups[data->group_count++];

        *split = *group;
        split->first_index = data->index_count;
        split->index_count = 0;
        group = split;
    }
    group->material = p->material;
}

static void init_material(ObjMaterial *material, const char *name)
{
    memset(material, 0, sizeof *material);
    snprintf(material->name, sizeof material->name, "%s", name);
    material->kd = v3(1.0f, 1.0f, 1.0f);
    material->opacity = 1.0f;
    material->roughness = 1.0f;
}

static int parse_mtl_line(Model *model, ObjMaterial **current, char *line, const char *path, int line_number)
{
    ObjMaterial *material = *current;
    const char *key;
    char *rest;
    float values[3];

    if (line[0] == '#') {
        // the glTF converter leaves the alpha mode and sidedness of a material in a comment
        if (material != NULL && strstr(line, "BLEND") != NULL) {
            material->blend = 1;
        }
        if (material != NULL && strstr(line, "doubleSided") != NULL) {
            material->double_sided = 1;
        }
        return 0;
    }

    key = line;
    rest = split_key(line);
    if (strcmp(key, "newmtl") == 0) {
        *current = &model->materials[model->material_count++];
        init_material(*current, rest);
        return 0;
    }
    if (material == NULL) {
        return 0;
    }

    if (strcmp(key, "Kd") == 0) {
        if (parse_floats(rest, values, 3) != 0) {
            return fail(path, line_number, "bad Kd");
        }
        material->kd = v3(values[0], values[1], values[2]);
    } else if (strcmp(key, "Ke") == 0) {
        if (parse_floats(rest, values, 3) != 0) {
            return fail(path, line_number, "bad Ke");
        }
        material->ke = v3(values[0], values[1], values[2]);
    } else if (strcmp(key, "d") == 0) {
        if (parse_floats(rest, values, 1) != 0) {
            return fail(path, line_number, "bad d");
        }
        // Blender writes d 0 for materials it means to show; zero reads as opaque
        material->opacity = values[0] == 0.0f ? 1.0f : values[0];
    } else if (strcmp(key, "Pm") == 0) {
        if (parse_floats(rest, values, 1) != 0) {
            return fail(path, line_number, "bad Pm");
        }
        material->metallic = values[0];
    } else if (strcmp(key, "Pr") == 0) {
        if (parse_floats(rest, values, 1) != 0) {
            return fail(path, line_number, "bad Pr");
        }
        material->roughness = values[0];
    } else if (strcmp(key, "map_Kd") == 0) {
        // the whole rest of the line is the map name, spaces included
        snprintf(material->diffuse_map, sizeof material->diffuse_map, "%s", rest);
    } else if (strcmp(key, "map_d") == 0) {
        snprintf(material->alpha_map, sizeof material->alpha_map, "%s", rest);
    } else if (strcmp(key, "map_Bump") == 0 || strcmp(key, "norm") == 0) {
        snprintf(material->normal_map, sizeof material->normal_map, "%s", rest);
    } else if (strcmp(key, "map_Pr") == 0) {
        snprintf(material->metal_rough_map, sizeof material->metal_rough_map, "%s", rest);
    } else if (strcmp(key, "map_Ke") == 0) {
        snprintf(material->emissive_map, sizeof material->emissive_map, "%s", rest);
    }

    return 0;
}

static int load_mtl(Parser *p, const char *name)
{
    Model *model = p->model;
    char path[OBJ_PATH_MAX];
    char line[LINE_CAPACITY];
    ObjMaterial *current = NULL;
    FILE *file;
    int count = 0;
    int line_number = 0;
    int status = 0;

    if (snprintf(path, sizeof path, "%s/%s", model->directory, name) >= (int)sizeof path) {
        return fail(p->path, p->line, "mtllib path is too long");
    }
    file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "%s:%d: cannot open %s: %s, continuing without its materials\n", p->path, p->line, path,
                strerror(errno));
        return 0;
    }

    while (fgets(line, sizeof line, file) != NULL) {
        if (strncmp(line, "newmtl", 6) == 0) {
            count++;
        }
    }
    rewind(file);
    if (count > 0) {
        ObjMaterial *grown = realloc(model->materials, sizeof *grown * (size_t)(model->material_count + count));

        if (grown == NULL) {
            fclose(file);
            fprintf(stderr, "out of memory loading %s\n", path);
            return -1;
        }
        model->materials = grown;
    }

    while (status == 0 && fgets(line, sizeof line, file) != NULL) {
        line_number++;
        if (line_overflowed(line, sizeof line)) {
            status = fail(path, line_number, "line too long");
        } else {
            status = parse_mtl_line(model, &current, line, path, line_number);
        }
    }
    if (status == 0 && ferror(file)) {
        status = fail(path, line_number, "read error");
    }
    fclose(file);

    return status;
}

static int parse_line(Parser *p, char *line)
{
    const char *key = line;
    char *rest = split_key(line);

    if (strcmp(key, "v") == 0) {
        return add_position(p, rest);
    }
    if (strcmp(key, "vt") == 0) {
        return add_uv(p, rest);
    }
    if (strcmp(key, "vn") == 0) {
        return add_normal(p, rest);
    }
    if (strcmp(key, "f") == 0) {
        return add_face(p, rest);
    }
    if (strcmp(key, "g") == 0) {
        begin_group(p, rest);
    } else if (strcmp(key, "usemtl") == 0) {
        use_material(p, rest);
    } else if (strcmp(key, "mtllib") == 0) {
        return load_mtl(p, rest);
    }

    return 0;
}

static Vec3 perpendicular(Vec3 n)
{
    Vec3 axis = v3(0.0f, 0.0f, 1.0f);

    if (fabsf(n.x) < fabsf(n.y) && fabsf(n.x) < fabsf(n.z)) {
        axis = v3(1.0f, 0.0f, 0.0f);
    } else if (fabsf(n.y) < fabsf(n.z)) {
        axis = v3(0.0f, 1.0f, 0.0f);
    }

    return v3_normalize(v3_cross(n, axis));
}

static void finish(Parser *p)
{
    MeshData *data = &p->model->data;
    int i;

    for (i = 0; i < p->position_count; i++) {
        p->position_normals[i] = v3_normalize(p->position_normals[i]);
    }

    for (i = 0; i < data->vertex_count; i++) {
        MeshVertex *vertex = &data->vertices[i];
        const Pending *pending = &p->pending[i];
        const Vec3 normal = pending->corner.vn >= 0 ? vertex->normal : p->position_normals[pending->corner.v];
        const Vec3 tangent = v3_sub(pending->tangent, v3_scale(normal, v3_dot(normal, pending->tangent)));

        vertex->normal = normal;
        if (v3_length(tangent) < 1e-6f) {
            vertex->tangent = perpendicular(normal);
            vertex->tangent_sign = 1.0f;
        } else {
            vertex->tangent = v3_normalize(tangent);
            vertex->tangent_sign = v3_dot(v3_cross(normal, vertex->tangent), pending->bitangent) < 0.0f ? -1.0f : 1.0f;
        }
    }

    if (data->groups[data->group_count - 1].index_count == 0) {
        data->group_count--;
    }
}

static int split_directory(const char *path, char *directory, size_t size)
{
    const char *separator = strrchr(path, '/');
    int length;
#ifdef _WIN32
    const char *backslash = strrchr(path, '\\');

    if (backslash != NULL && (separator == NULL || backslash > separator)) {
        separator = backslash;
    }
#endif

    if (separator == NULL) {
        length = snprintf(directory, size, ".");
    } else {
        length = snprintf(directory, size, "%.*s", (int)(separator - path), path);
    }

    return length >= 0 && (size_t)length < size ? 0 : -1;
}

int obj_load(Model *model, const char *path)
{
    Parser parser;
    Counts counts;
    char line[LINE_CAPACITY];
    FILE *file;
    int status = 0;

    memset(model, 0, sizeof *model);
    memset(&parser, 0, sizeof parser);
    parser.model = model;
    parser.path = path;
    parser.material = -1;

    if (split_directory(path, model->directory, sizeof model->directory) != 0) {
        fprintf(stderr, "model path is too long: %s\n", path);
        return -1;
    }
    file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "cannot open %s: %s\n", path, strerror(errno));
        return -1;
    }

    count_lines(file, &counts);
    if (allocate(&parser, &counts) != 0) {
        fprintf(stderr, "out of memory loading %s\n", path);
        status = -1;
    }
    while (status == 0 && fgets(line, sizeof line, file) != NULL) {
        parser.line++;
        if (line_overflowed(line, sizeof line)) {
            status = fail(path, parser.line, "line too long");
        } else {
            status = parse_line(&parser, line);
        }
    }
    if (status == 0 && ferror(file)) {
        status = fail(path, parser.line, "read error");
    }
    fclose(file);

    if (status == 0) {
        finish(&parser);
    }
    free_parser(&parser);
    if (status != 0) {
        obj_free(model);
    }

    return status;
}

void obj_free(Model *model)
{
    mesh_data_free(&model->data);
    free(model->materials);
    memset(model, 0, sizeof *model);
}
