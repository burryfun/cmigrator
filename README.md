# cmigrator
A simple C-based utility for automatically executing and tracking SQL script migrations in a PostgreSQL database.

## Usage
``` bash
./migrator postgresql://user:password@localhost:5432/mydb
```
If no connection string is provided, the default is:
```
postgresql://postgres:admin@localhost:5432/db
```
Place your SQL files in the scripts/ directory.

### Requirements

- PostgreSQL client library (libpq)
