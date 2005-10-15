create table roadmap_categories (
   name   varchar(16) primary key,
   label  varchar(64),
   parent varchar(16),
   icon   varchar(32)
);

