## Database diagram(s)

Database schema is defined in [`datastore/schema.sql`](../datastore/schema.sql).

### Overview
The diagram in this section includes the main tables in the gatekeeper database.The relationships between tables are many-to-many, with the junction table named on the connecting lines.

```mermaid
erDiagram
    ACCESS-POLICIES {
        text    _id  PK
        integer _rev "not null"
        jsonb   rules
    }

    COLLECTIONS {
        text     _id
        integer _rev
        text    name "not null"
    }

    IDENTITIES {
        text    _id   PK
        integer _rev  "not null"
        text    sub   "not null, unique"
        jsonb   attrs
    }

      RBAC-POLICIES {
        text    _id  PK
        integer _rev "not null"
        text    name "not null"
        jsonb   rules
    }

    ROLES {
        text    _id  PK
        integer _rev "not null"
        text    name "not null"
        text[]  permissions
    }

    COLLECTIONS }o--o{ IDENTITIES : "collection_identities"
    COLLECTIONS }o--o{ ACCESS-POLICIES : "access-policies_collections"
    IDENTITIES }o--o{ ACCESS-POLICIES : "access-policies_identities"
    IDENTITIES }o--o{ RBAC-POLICIES : "rbac-policies_identities"
    COLLECTIONS }o--o{ RBAC-POLICIES : "rbac-policies_collections"
    RBAC-POLICIES }o--o{ ROLES : "rbac-policies_roles"
```

### Collection identities diagram

```mermaid
erDiagram
    IDENTITIES ||--o{ COLLECTIONS_IDENTITIES : "is member of"
    COLLECTIONS ||--o{ COLLECTIONS_IDENTITIES : includes
```

### Access Policies diagram
```mermaid
erDiagram
    ACCESS-POLICIES ||--o{ ACCESS-POLICIES_COLLECTIONS : has
    COLLECTIONS ||--o{ ACCESS-POLICIES_COLLECTIONS : "assigned to"

    IDENTITIES ||--o{ ACCESS-POLICIES_IDENTITIES : has
    ACCESS-POLICIES ||--o{ ACCESS-POLICIES_IDENTITIES : "assigned to"
```

### RBAC policies diagram
```mermaid
erDiagram
    COLLECTIONS ||--o{ RBAC-POLICIES_COLLECTIONS : has
    RBAC-POLICIES ||--o{ RBAC-POLICIES_COLLECTIONS : "assigned to"

    IDENTITIES ||--o{ RBAC-POLICIES_IDENTITIES : has
    RBAC-POLICIES ||--o{ RBAC-POLICIES_IDENTITIES : "assigned to"

    RBAC-POLICIES ||--o{ RBAC-POLICIES_ROLES : has
    ROLES ||--o{ RBAC-POLICIES_ROLES : "belongs to"
```
