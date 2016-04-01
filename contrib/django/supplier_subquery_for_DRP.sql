Issues with query below:
- doesnt cover item distributions
- performance will be very slow for filtering and sorting
- django orm freaks out when filtering on this annotation

Proper solution:
Add a computed field to the buffer table, with a text description of the supply.

supplier_subquery = '''
  select string_agg(
    case when priority <> 1 then priority || ': ' else '' end
    || supplier_id
    || case when effective_start is not null then ' from ' || to_char(effective_start, 'YYYY/MM/DD') else '' end
    || case when effective_end is not null then ' till ' || to_char(effective_end, 'YYYY/MM/DD') else '' end,
    ', ' order by priority asc
    )
  from itemsupplier
  inner join buffer buffer_sub
    on out_inventoryplanning.buffer_id = buffer_sub.name
  inner join location location_sub
    on buffer_sub.location_id = location_sub.name
  inner join item item_sub
    on buffer_sub.item_id = item_sub.name
  where (itemsupplier.location_id is null or exists (
      select 1 from location location_2
      where itemsupplier.location_id = location_2.name
      and location_sub.lft >= location_2.lft and location_sub.lft < location_2.rght
      )) and
    (itemsupplier.item_id is null or exists (
      select 1 from item item_2
      where itemsupplier.item_id = item_2.name
      and item_sub.lft >= item_2.lft and item_sub.lft < item_2.rght
      ))'''